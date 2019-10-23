#include "schema_checker.h"
#include "update_plan.h"
#include "update_stmt.h"
#include "table_space.h"
#include "expression.h"
#include "expr_stmt.h"
#include "filter.h"
#include "object.h"
#include "update.h"
#include "stmt.h"
#include "error.h"
#include "log.h"

using namespace CatDB::Sql;
using namespace CatDB::Common;
using namespace CatDB::Parser;
using namespace CatDB::Storage;

UpdatePlan::UpdatePlan()
{

}

UpdatePlan::~UpdatePlan()
{

}

Plan_s UpdatePlan::make_update_plan(const Stmt_s& lex_insert_stmt)
{
	UpdatePlan* plan = new UpdatePlan;
	plan->set_lex_stmt(lex_insert_stmt);
	return Plan_s(plan);
}
u32 UpdatePlan::execute()
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
	while ((ret = root_operator->get_next_row(row_access)) == SUCCESS)
	{
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

u32 UpdatePlan::build_plan()
{
	if (!lex_stmt || lex_stmt->stmt_type() != Stmt::Update)
	{
		Log(LOG_ERR, "UpdatePlan", "error lex stmt when build update plan");
		return ERROR_LEX_STMT;
	}
	SchemaChecker_s checker = SchemaChecker::make_schema_checker();
	assert(checker);
	UpdateStmt* lex = dynamic_cast<UpdateStmt*>(lex_stmt.get());
	assert(lex->table);
	//������������
	TableStmt* table = dynamic_cast<TableStmt*>(lex->table.get());
	database = table->database;
	table_name = table->table_name;
	TableSpace_s table_space = TableSpace::make_table_space(table_name, database);
	assert(table_space);
	//��where�Ӿ�ת��Ϊfilter
	Filter_s filter;
	u32 ret = resolve_filter(lex->where_stmt, filter);
	if (ret != SUCCESS) {
		Log(LOG_ERR, "UpdatePlan", "create filter from where stmt error:%s", err_string(ret));
		return ret;
	}
	Row_s new_row;
	ret = resolve_update_row(lex->update_asgn_stmt, new_row);
	if (ret != SUCCESS) {
		Log(LOG_ERR, "UpdatePlan", "create update row from asign stmt error:%s", err_string(ret));
		return ret;
	}
	root_operator = Update::make_update(table_space, new_row, filter);
	return SUCCESS;
}

u32 UpdatePlan::optimizer()
{
	//TODO:add later
	return SUCCESS;
}

Plan::PlanType UpdatePlan::type() const
{
	return Plan::DELETE;
}

u32 UpdatePlan::resolve_filter(const Stmt_s& where_stmt, Filter_s& filter)
{
	if (!where_stmt) {
		filter.reset();
		return SUCCESS;
	}
	Expression_s expr;
	u32 ret = resolve_expr(where_stmt, expr);
	if (ret != SUCCESS) {
		return ret;
	}
	filter = Filter::make_filter(expr);
	return SUCCESS;
}

u32 UpdatePlan::resolve_expr(const Stmt_s& stmt, Expression_s& expr)
{
	if (stmt->stmt_type() != Stmt::Expr) {
		return ERROR_LEX_STMT;
	}
	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get());

	SchemaChecker_s checker = SchemaChecker::make_schema_checker();
	assert(checker);
	
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
		if (!checker->have_column(database, table_name, column_stmt->column)) {
			Log(LOG_ERR, "UpdatePlan", "%s.%s do not have column %s", 
				database.c_str(), table_name.c_str(), column_stmt->column.c_str());
			return COLUMN_NOT_EXISTS;
		}
		col_desc = checker->get_column_desc(database, table_name, column_stmt->column);
		expr = ColumnExpression::make_column_expression(col_desc);
		ret = SUCCESS;
		break;
	case ExprStmt::Query:
		Log(LOG_ERR, "UpdatePlan", "subquery in update`s where stmt not support yet");
		ret = ERROR_LEX_STMT;
		break;
	case ExprStmt::List:
		Log(LOG_ERR, "UpdatePlan", "list stmt in update`s where stmt not support yet");
		ret = ERROR_LEX_STMT;
		break;
	case ExprStmt::Unary:
		unary_stmt = dynamic_cast<UnaryExprStmt*>(expr_stmt);
		ret = resolve_expr(unary_stmt->expr_stmt, first_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create unary expression`s first expression failed");
			break;
		}
		expr = UnaryExpression::make_unary_expression(first_expr, unary_stmt->op_type);
		ret = SUCCESS;
		break;
	case ExprStmt::Binary:
		binary_stmt = dynamic_cast<BinaryExprStmt*>(expr_stmt);
		ret = resolve_expr(binary_stmt->first_expr_stmt, first_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create binary expression`s first expression failed");
			break;
		}
		ret = resolve_expr(binary_stmt->second_expr_stmt, second_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create binary expression`s second expression failed");
			break;
		}
		expr = BinaryExpression::make_binary_expression(first_expr, second_expr, binary_stmt->op_type);
		ret = SUCCESS;
		break;
	case ExprStmt::Ternary:
		ternary_stmt = dynamic_cast<TernaryExprStmt*>(expr_stmt);
		ret = resolve_expr(ternary_stmt->first_expr_stmt, first_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create ternary expression`s first expression failed");
			break;
		}
		ret = resolve_expr(ternary_stmt->second_expr_stmt, second_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create ternary expression`s second expression failed");
			break;
		}
		ret = resolve_expr(ternary_stmt->third_expr_stmt, third_expr);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create ternary expression`s third expression failed");
			break;
		}
		expr = TernaryExpression::make_ternary_expression(first_expr, second_expr, third_expr, ternary_stmt->op_type);
		ret = SUCCESS;
		break;
	default:
		Log(LOG_ERR, "UpdatePlan", "unknown expr stmt in update`s where stmt");
		ret = ERROR_LEX_STMT;
	}
	return ret;
}

u32 UpdatePlan::resolve_update_row(const Stmt_s& asgn_stmt, Row_s& row)
{
	if (asgn_stmt->stmt_type() != Stmt::Expr) {
		return ERROR_LEX_STMT;
	}
	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(asgn_stmt.get());
	if (expr_stmt->expr_stmt_type() != ExprStmt::List) {
		return ERROR_LEX_STMT;
	}
	ListStmt* list_stmt = dynamic_cast<ListStmt*>(expr_stmt);
	u32 column_count = list_stmt->stmt_list.size();
	RowDesc row_desc(column_count);
	row = Row::make_row(row_desc);
	for (u32 i = 0; i < column_count; ++i) {
		ColumnDesc col_desc;
		Object_s cell;
		u32 ret = resolve_cell(list_stmt->stmt_list[i], col_desc, cell);
		if (ret != SUCCESS) {
			Log(LOG_ERR, "UpdatePlan", "create %uth column failed", i);
			return ret;
		}
		row_desc.set_column_desc(i, col_desc);
		row->set_cell(i, cell);
	}
	return SUCCESS;
}

u32 UpdatePlan::resolve_cell(const Stmt_s& asign_stmt, ColumnDesc&col_desc, Object_s& cell)
{
	if (asign_stmt->stmt_type() != Stmt::Expr) {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must be expression stmt");
		return ERROR_LEX_STMT;
	}
	ExprStmt* expr_stmt = dynamic_cast<ExprStmt*>(asign_stmt.get());
	if (expr_stmt->expr_stmt_type() != ExprStmt::Binary) {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must be binary expression stmt");
		return ERROR_LEX_STMT;
	}
	BinaryExprStmt* binary_stmt = dynamic_cast<BinaryExprStmt*>(expr_stmt);
	if (binary_stmt->op_type != ExprStmt::OP_EQ) {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must be equal expression stmt");
		return ERROR_LEX_STMT;
	}
	if (binary_stmt->first_expr_stmt->stmt_type() != Stmt::Expr
		|| binary_stmt->second_expr_stmt->stmt_type() != Stmt::Expr) {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must be expression stmt");
		return ERROR_LEX_STMT;
	}
	ColumnStmt* column_stmt = nullptr;
	ConstStmt* const_stmt = nullptr;
	ExprStmt* first_expr = dynamic_cast<ExprStmt*>(binary_stmt->first_expr_stmt.get());
	ExprStmt* second_expr = dynamic_cast<ExprStmt*>(binary_stmt->second_expr_stmt.get());
	if (first_expr->expr_stmt_type() == ExprStmt::Column) {
		column_stmt = dynamic_cast<ColumnStmt*>(first_expr);
	}
	else if (first_expr->expr_stmt_type() == ExprStmt::Const) {
		const_stmt = dynamic_cast<ConstStmt*>(first_expr);
	}
	else {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must have a column stmt and a const stmt");
		return ERROR_LEX_STMT;
	}

	if (second_expr->expr_stmt_type() == ExprStmt::Column) {
		column_stmt = dynamic_cast<ColumnStmt*>(second_expr);
	}
	else if (second_expr->expr_stmt_type() == ExprStmt::Const) {
		const_stmt = dynamic_cast<ConstStmt*>(second_expr);
	}
	else {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must have a column stmt and a const stmt");
		return ERROR_LEX_STMT;
	}
	if (!column_stmt || !const_stmt) {
		Log(LOG_ERR, "UpdatePlan", "update asign stmt must have a column stmt and a const stmt");
		return ERROR_LEX_STMT;
	}
	SchemaChecker_s checker = SchemaChecker::make_schema_checker();
	assert(checker);
	if (!checker->have_column(database, table_name, column_stmt->column)) {
		Log(LOG_ERR, "UpdatePlan", "%s.%s do not have column %s",
			database.c_str(), table_name.c_str(), column_stmt->column.c_str());
		return COLUMN_NOT_EXISTS;
	}
	col_desc = checker->get_column_desc(database, table_name, column_stmt->column);
	cell = const_stmt->value;
	return SUCCESS;
}