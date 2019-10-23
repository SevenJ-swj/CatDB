#include "schema_checker.h"
#include "select_plan.h"
#include "select_stmt.h"
#include "table_space.h"
#include "expression.h"
#include "expr_stmt.h"
#include "filter.h"
#include "object.h"
#include "table_scan.h"
#include "hash_distinct.h"
#include "hash_group.h"
#include "hash_set.h"
#include "scalar_group.h"
#include "table_space.h"
#include "hash_join.h"
#include "limit.h"
#include "sort.h"
#include "stmt.h"
#include "row.h"
#include "error.h"
#include "log.h"

using namespace CatDB::Sql;
using namespace CatDB::Common;
using namespace CatDB::Parser;
using namespace CatDB::Storage;

SelectPlan::SelectPlan()
{

}

SelectPlan::~SelectPlan()
{

}

Plan_s SelectPlan::make_select_plan(const Stmt_s& lex_insert_stmt)
{
	SelectPlan* plan = new SelectPlan;
	plan->set_lex_stmt(lex_insert_stmt);
	return Plan_s(plan);
}
u32 SelectPlan::execute()
{
	if (!root_operator) {
		return PLAN_NOT_BUILD;
	}
	u32 ret;
	ret = root_operator->open();
	if (ret != SUCCESS) {
		result = Error::make_object(ret);
		return ret;
	}

	affect_rows_ = 0;
	Row_s row;
	while ((ret = root_operator->get_next_row(row)) == SUCCESS)
	{
		//save row to result
		++affect_rows_;
	}
	u32 ret2 = root_operator->close();
	if (ret != NO_MORE_ROWS) {
		result = Error::make_object(ret);
		return ret;
	}
	else if (ret2 != SUCCESS) {
		result = Error::make_object(ret2);
		return ret2;
	}
	else {
		result = Error::make_object(SUCCESS);
		return SUCCESS;
	}
}

u32 SelectPlan::build_plan()
{
	if (!lex_stmt || lex_stmt->stmt_type() != Stmt::Select)
	{
		if (lex_stmt && lex_stmt->stmt_type() == Stmt::Expr)
		{
			Log(LOG_ERR, "SelectPlan", "set operation not support yet");
		}
		Log(LOG_ERR, "SelectPlan", "error lex stmt when build select plan");
		return ERROR_LEX_STMT;
	}
	SchemaChecker_s checker = SchemaChecker::make_schema_checker();
	assert(checker);
	SelectStmt* lex = dynamic_cast<SelectStmt*>(lex_stmt.get());
	u32 ret = get_ref_tables(lex->from_stmts);
	if (ret != SUCCESS) {
		Log(LOG_ERR, "SelectPlan", "error stmt in from stmts");
		return ret;
	}
	
	return SUCCESS;
}

u32 SelectPlan::optimizer()
{
	//TODO:add later
	return SUCCESS;
}

Plan::PlanType SelectPlan::type() const
{
	return Plan::SELECT;
}

/*
 * ����where�Ӿ�飬�зֳ�and���ӵ�ν�ʷ���
 */
u32 SelectPlan::resolve_where_stmt(const Stmt_s& where_stmt)
{
	if (where_stmt->stmt_type() != Stmt::Expr) {
		return ERROR_LEX_STMT;
	}

	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(where_stmt.get());
	//�����and���ӵ�ν��
	if (expr_stmt->expr_stmt_type() == ExprStmt::Binary) {
		BinaryExprStmt* binary_stmt = dynamic_cast<BinaryExprStmt*>(expr_stmt);
		if (binary_stmt->op_type == ExprStmt::OP_AND) {
			//�ݹ����������
			u32 ret = resolve_where_stmt(binary_stmt->first_expr_stmt);
			if (ret != SUCCESS) {
				Log(LOG_ERR, "SelectPlan", "create binary expression`s first expression failed");
				return ret;
			}
			//�ݹ����������
			ret = resolve_where_stmt(binary_stmt->second_expr_stmt);
			if (ret != SUCCESS) {
				Log(LOG_ERR, "SelectPlan", "create binary expression`s second expression failed");
				return ret;
			}
			return SUCCESS;
		}
	}
	//�ָ��С����
	return resolve_simple_stmt(where_stmt);
}

/*
 * ������С����䣬�ֳ����������
 * ������ֵ����ν�ʡ�������ͨ����ν��
 * ��������ν�ʡ���ͨ����ν��
 */
u32 SelectPlan::resolve_simple_stmt(const Stmt_s& stmt)
{
	if (stmt->stmt_type() != Stmt::Expr) {
		return ERROR_LEX_STMT;
	}

	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get());
	if (expr_stmt->expr_stmt_type() == ExprStmt::Binary) {
		BinaryExprStmt* binary_stmt = dynamic_cast<BinaryExprStmt*>(expr_stmt);
		ColumnStmt* first_column = resolve_column_stmt(binary_stmt->first_expr_stmt);
		ColumnStmt* second_column = resolve_column_stmt(binary_stmt->second_expr_stmt);
		//������join����
		if (first_column && second_column) {
			TableStmt* first_table = nullptr;
			TableStmt* second_table = nullptr;
			u32 ret = who_have_column(first_column, first_table);
			if (ret != SUCCESS) {
				return ret;
			}
			ret = who_have_column(second_column, second_table);
			if (ret != SUCCESS) {
				return ret;
			}
			//���ű����й�����������
			if (first_table->alias_name != second_table->alias_name) {
				if (binary_stmt->op_type == ExprStmt::OP_EQ) {
					Expression_s expr;
					u32 ret = resolve_expr(stmt, expr);
					if (ret != SUCCESS) {
						return ret;
					}
					add_join_equal_cond(JoinableTables(first_table, second_table), expr);
					add_join_cond(JoinableTables(first_table, second_table), expr);
				}
				else {
					Expression_s expr;
					u32 ret = resolve_expr(stmt, expr);
					if (ret != SUCCESS) {
						return ret;
					}
					add_join_cond(JoinableTables(first_table, second_table), expr);
				}
			}
		}
	}
	//����Ƿ��ǵ��ű��Ĺ���ν��
	Expression_s expr;
	u32 ret = resolve_expr(stmt, expr);
	if (ret != SUCCESS) {
		return ret;
	}
	TableStmt* table = nullptr;
	if (is_table_filter(stmt, table)) {
		add_table_filter(table, expr);
	}
	else {
		//��ͨ����ν�����������join��ʹ��
		make_and_expression(filter_after_join, expr);
	}
	return SUCCESS;
}

/*
 * �����ǰν��������󷵻��У����򷵻�null
 */
ColumnStmt * SelectPlan::resolve_column_stmt(const Stmt_s & stmt)
{
	if (stmt->stmt_type() != Stmt::Expr) {
		return nullptr;
	}
	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get());
	if (expr_stmt->expr_stmt_type() == ExprStmt::Column) {
		return dynamic_cast<ColumnStmt*>(expr_stmt);
	}
	else {
		return nullptr;
	}
}

/*
 * ��ν������ת��Ϊ����ʽ
 */
u32 SelectPlan::resolve_expr(const Stmt_s& stmt, Expression_s& expr)
{
	if (stmt->stmt_type() != Stmt::Expr) {
		return ERROR_LEX_STMT;
	}

	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get());
	Expression_s first_expr, second_expr, third_expr;
	ConstStmt* const_stmt;
	ColumnStmt* column_stmt;
	ColumnDesc col_desc;
	UnaryExprStmt* unary_stmt;
	BinaryExprStmt* binary_stmt;
	TernaryExprStmt* ternary_stmt;
	u32 ret;

	switch (expr_stmt->expr_stmt_type())
	{
	case ExprStmt::Const:
		const_stmt = dynamic_cast<ConstStmt*>(expr_stmt);
		expr = ConstExpression::make_const_expression(const_stmt->value);
		ret = SUCCESS;
		break;
	case ExprStmt::Column:
		column_stmt = dynamic_cast<ColumnStmt*>(expr_stmt);
		ret = resolve_column_desc(column_stmt, col_desc);
		if (ret != SUCCESS) {
			break;
		}
		expr = ColumnExpression::make_column_expression(col_desc);
		ret = SUCCESS;
		break;
	case ExprStmt::Query:
		Log(LOG_ERR, "SelectPlan", "subquery in select`s where stmt not support yet");
		ret = ERROR_LEX_STMT;
		break;
	case ExprStmt::List:
		Log(LOG_ERR, "SelectPlan", "list stmt in select`s where stmt not support yet");
		ret = ERROR_LEX_STMT;
		break;
	case ExprStmt::Unary:
		unary_stmt = dynamic_cast<UnaryExprStmt*>(expr_stmt);
		ret = resolve_expr(unary_stmt->expr_stmt, first_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "create unary expression`s first expression failed");
			break;
		}
		expr = UnaryExpression::make_unary_expression(first_expr, unary_stmt->op_type);
		ret = SUCCESS;
		break;
	case ExprStmt::Binary:
		binary_stmt = dynamic_cast<BinaryExprStmt*>(expr_stmt);

		ret = resolve_expr(binary_stmt->first_expr_stmt, first_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "create binary expression`s first expression failed");
			break;
		}
		ret = resolve_expr(binary_stmt->second_expr_stmt, second_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "create binary expression`s second expression failed");
			break;
		}
		expr = BinaryExpression::make_binary_expression(first_expr, second_expr, binary_stmt->op_type);
		ret = SUCCESS;
		break;
	case ExprStmt::Ternary:
		ternary_stmt = dynamic_cast<TernaryExprStmt*>(expr_stmt);
		ret = resolve_expr(ternary_stmt->first_expr_stmt, first_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "create ternary expression`s first expression failed");
			break;
		}
		ret = resolve_expr(ternary_stmt->second_expr_stmt, second_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "create ternary expression`s second expression failed");
			break;
		}
		ret = resolve_expr(ternary_stmt->third_expr_stmt, third_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "create ternary expression`s third expression failed");
			break;
		}
		expr = TernaryExpression::make_ternary_expression(first_expr, second_expr, third_expr, ternary_stmt->op_type);
		ret = SUCCESS;
		break;
	default:
		Log(LOG_ERR, "SelectPlan", "unknown expr stmt in select`s where stmt");
		ret = ERROR_LEX_STMT;
	}
	return ret;
}
/*
 * ��ǰν�������Ƿ��ǻ�������ν�ʣ�
 * ���ν����ֻ������һ�ű����л��߶��ǳ�����
 * ���ʾ�ǻ�������ν�ʣ������ػ���
 */
bool SelectPlan::is_table_filter(const Stmt_s & stmt, TableStmt *& table)
{
	if (stmt->stmt_type() != Stmt::Expr) {
		return false;
	}

	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get());
	ColumnStmt* column_stmt;
	UnaryExprStmt* unary_stmt;
	BinaryExprStmt* binary_stmt;
	TernaryExprStmt* ternary_stmt;
	bool ret = true;

	switch (expr_stmt->expr_stmt_type())
	{
	case ExprStmt::Const:
		ret = true;
		break;
	case ExprStmt::Column:
		column_stmt = dynamic_cast<ColumnStmt*>(expr_stmt);
		//�ڴ�֮ǰû�г��ֹ������ν��
		if (table == nullptr) {
			u32 rt = who_have_column(column_stmt, table);
			if (rt != SUCCESS) {
				ret = false;
			}
			else {
				ret = true;
			}
		}
		//�ڴ�֮ǰ���ֹ������ν��
		else {
			TableStmt* tb = nullptr;
			u32 rt = who_have_column(column_stmt, tb);
			//��������ν�ʲ�����ͬһ�ű�����ʾ���ǻ�������ν��
			if (rt != SUCCESS || table != tb) {
				ret = false;
			}
			else {
				ret = true;
			}
		}
		break;
	case ExprStmt::Query:
		ret = false;
		break;
	case ExprStmt::List:
		ret = false;
		break;
	case ExprStmt::Unary:
		unary_stmt = dynamic_cast<UnaryExprStmt*>(expr_stmt);
		ret = is_table_filter(unary_stmt->expr_stmt, table);
		break;
	case ExprStmt::Binary:
		binary_stmt = dynamic_cast<BinaryExprStmt*>(expr_stmt);
		ret = is_table_filter(binary_stmt->first_expr_stmt, table);
		if (!ret) {
			break;
		}
		else {
			ret = is_table_filter(binary_stmt->second_expr_stmt, table);
		}
		break;
	case ExprStmt::Ternary:
		ternary_stmt = dynamic_cast<TernaryExprStmt*>(expr_stmt);
		ret = is_table_filter(ternary_stmt->first_expr_stmt, table);
		if (!ret) {
			break;
		}
		else {
			ret = is_table_filter(ternary_stmt->second_expr_stmt, table);
			if (!ret) {
				break;
			}
			else {
				ret = is_table_filter(ternary_stmt->third_expr_stmt, table);
			}
		}
		break;
	default:
		ret = false;
	}
	return ret;
}

/*
 * ��ȡ������
 */
u32 SelectPlan::resolve_column_desc(ColumnStmt * column_stmt, ColumnDesc & col_desc)
{
	SchemaChecker_s checker = SchemaChecker::make_schema_checker();
	assert(checker);
	TableStmt* table = nullptr;
	u32 ret = who_have_column(column_stmt, table);
	if (ret != SUCCESS) {
		return ret;
	}
	col_desc = checker->get_column_desc(table->database, table->table_name, column_stmt->column);
	return SUCCESS;
}

/*
 * ��from list������ָ�������ظ����ֱ�ʾfrom list����
 */
u32 SelectPlan::find_table(const String & table_name, TableStmt*& table)
{
	u32 find = 0;
	for (u32 i = 0; i < table_list.size(); ++i) {
		//������õı�û��ʹ�ñ������ڽ�����ʱ���ʹ����ʵ������ʾ����
		//��������ֻ��Ҫ�������ı���
		if (table_name == table_list[i]->alias_name) {
			++find;
			table = table_list[i];
		}
	}
	if (find == 1)
		return SUCCESS;
	else if (find == 0)
		return TABLE_NOT_EXISTS;
	else
		return TABLE_REDEFINE;
}

/*
 * ��from list����������ָ���еı�
 */
u32 SelectPlan::who_have_column(ColumnStmt * column_stmt, TableStmt *& table)
{
	//SQL��û��ָ���������ı�
	if (column_stmt->table.empty()) {
		u32 ret = who_have_column(column_stmt->column, table);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "parse column define error in where stmt:%s", err_string(ret));
			return ret;
		}
	}
	//SQL��ָ�����������ı�
	else {
		u32 ret = find_table(column_stmt->table, table);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "SelectPlan", "parse column define error in where stmt:%s", err_string(ret));
			return ret;
		}
	}
	return SUCCESS;
}
/*
 * ��from list����������ָ���еı�
 */
u32 SelectPlan::who_have_column(const String & column_name, TableStmt*& table)
{
	SchemaChecker_s checker = SchemaChecker::make_schema_checker();
	assert(checker);
	u32 find = 0;
	for (u32 i = 0; i < table_list.size(); ++i) {
		if (checker->have_column(table_list[i]->database, table_list[i]->table_name, column_name)) {
			++find;
			table = table_list[i];
		}
	}
	if (find == 1)
		return SUCCESS;
	else if (find == 0)
		return TABLE_NOT_EXISTS;
	else
		return TABLE_REDEFINE;
}
/*
 * ��from �����н��������������˵ı�
 */
u32 SelectPlan::get_ref_tables(const Stmt_s & from_stmt)
{
	if (from_stmt->stmt_type() != Stmt::Expr) {
		return ERROR_LEX_STMT;
	}
	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(from_stmt.get());
	if (expr_stmt->expr_stmt_type() != ExprStmt::List) {
		return ERROR_LEX_STMT;
	}
	ListStmt* list_stmt = dynamic_cast<ListStmt*>(expr_stmt);
	for (u32 i = 0; i < list_stmt->stmt_list.size(); ++i) {
		if (list_stmt->stmt_list[i]->stmt_type() != Stmt::Expr) {
			return ERROR_LEX_STMT;
		}
		ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(list_stmt->stmt_list[i].get());
		if (expr_stmt->expr_stmt_type() == ExprStmt::Query) {
			Log(LOG_ERR, "SelectPlan", "subquery in from stmt not support yet");
			return ERROR_LEX_STMT;
		}
		else if (expr_stmt->expr_stmt_type() != ExprStmt::Table) {
			return ERROR_LEX_STMT;
		}
		TableStmt* table = dynamic_cast<TableStmt*>(expr_stmt);
		table_list.push_back(table);
	}
	return SUCCESS;
}
/*
 * ����ָ�����ű�������ν��
 */
u32 SelectPlan::search_jon_info(const JoinableTables & join_tables, JoinConditions & join_cond)
{
	u32 ret = JOIN_TABLES_NOT_EXISTS;
	for (auto iter = join_info.cbegin(); iter != join_info.cend(); ++iter) {
		if (iter->first.first->alias_name == join_tables.first->alias_name
			&& iter->first.second->alias_name == join_tables.second->alias_name) {
			join_cond = iter->second;
			ret = SUCCESS;
			break;
		}
		else if (iter->first.second->alias_name == join_tables.first->alias_name
			&& iter->first.first->alias_name == join_tables.second->alias_name) {
			join_cond = iter->second;
			ret = SUCCESS;
			break;
		}
	}
	return ret;
}
/*
 * ����ָ�����ű�������ν��
 */
u32 SelectPlan::add_join_cond(const JoinableTables & join_tables, const Expression_s & expr)
{
	JoinConditions join_cond;
	u32 ret = search_jon_info(join_tables, join_cond);
	if (ret == SUCCESS) {
		make_and_expression(join_cond.first, expr);
	}
	else {
		join_info[join_tables] = JoinConditions(expr, Expression_s());
	}
	return SUCCESS;
}
/*
 * ����ָ�����ű��ĵ�ֵ����ν��
 */
u32 SelectPlan::add_join_equal_cond(const JoinableTables & join_tables, const Expression_s & expr)
{
	JoinConditions join_cond;
	u32 ret = search_jon_info(join_tables, join_cond);
	if (ret == SUCCESS) {
		make_and_expression(join_cond.second, expr);
	}
	else {
		join_info[join_tables] = JoinConditions(Expression_s(), expr);
	}
	return SUCCESS;
}
/*
 * ���ӻ�������ν��
 */
u32 SelectPlan::add_table_filter(TableStmt * table, const Expression_s & filter)
{
	if (!table) {
		for (u32 i = 0; i < table_list.size(); ++i) {
			add_table_filter(table_list[i], filter);
		}
	}
	else {
		if (table_filters.find(table) == table_filters.cend()) {
			table_filters[table] = filter;
		}
		else {
			make_and_expression(table_filters[table], filter);
		}
	}
	return SUCCESS;
}
/*
 * ���ܣ�expr = expr and other
 */
u32 SelectPlan::make_and_expression(Expression_s & expr, const Expression_s & other)
{
	if (!expr) {
		expr = other;
		return SUCCESS;
	}
	Expression_s and_expr = BinaryExpression::make_binary_expression(expr, other, ExprStmt::OP_AND);
	expr = and_expr;
	return SUCCESS;
}

u32 SelectPlan::choos_best_join_order()
{
	return SUCCESS;
}