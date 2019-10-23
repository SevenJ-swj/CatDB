#ifndef SELECT_PLAN_H
#define SELECT_PLAN_H
#include "type.h"
#include "plan.h"

namespace CatDB {
	namespace Parser {
		DECLARE(Stmt);
		DECLARE(TableStmt);
		DECLARE(ColumnStmt);
	}
	namespace Common {
		DECLARE(Row);
	}
	namespace Sql {
		DECLARE(Plan);
		DECLARE(Filter);
		DECLARE(Expression);
		using Common::Row_s;
		using Parser::Stmt_s;
		using Sql::Filter_s;
		using Sql::Expression_s;
		using Parser::TableStmt;
		using Parser::ColumnStmt;

		class SelectPlan : public Plan
		{
		protected:
			SelectPlan();
		public:
			~SelectPlan();
			static Plan_s make_select_plan(const Stmt_s& lex_insert_stmt);
			u32 execute();
			u32 build_plan();
			u32 optimizer();
			PlanType type() const;
		private:
			typedef Pair<TableStmt*, TableStmt*> JoinableTables;
			typedef Pair<Expression_s, Expression_s> JoinConditions;
			//����where�Ӿ䣬��ֳ�һ��һ��and���ӵ�stmt
			u32 resolve_where_stmt(const Stmt_s& where_stmt);
			//��������stmt������������ν�ʣ������ǻ������ν�ʣ�Ҳ��������ͨ�Ĺ���ν��
			u32 resolve_simple_stmt(const Stmt_s& expr_stmt);
			//��ǰ����������������򷵻������������򷵻�null
			ColumnStmt* resolve_column_stmt(const Stmt_s& stmt);
			//������ת���ɱ��ʽ
			u32 resolve_expr(const Stmt_s& expr_stmt, Expression_s& expr);
			//�жϵ�ǰν�ʿ��Ƿ��ǻ������ν��
			bool is_table_filter(const Stmt_s& expr_stmt, TableStmt*& table);
			//�����������ȡ������
			u32 resolve_column_desc(ColumnStmt* column_stmt, ColumnDesc& col_desc);
			//��from list��������
			u32 find_table(const String& table_name, TableStmt*& table);
			//��from list����������ָ���еı�
			u32 who_have_column(ColumnStmt* column_stmt, TableStmt*& table);
			//��from list����������ָ���еı�
			u32 who_have_column(const String& column_name, TableStmt*& table);
			//����from����Ϊtable list
			u32 get_ref_tables(const Stmt_s& from_stmt);
			//�������ű����������ű������ν��
			u32 search_jon_info(const JoinableTables& join_tables, JoinConditions& join_cond);
			//������ű������ν��
			u32 add_join_cond(const JoinableTables& join_tables, const Expression_s& expr);
			//������ű�ĵ�ֵ����ν��
			u32 add_join_equal_cond(const JoinableTables& join_tables, const Expression_s& expr);
			//��ӻ������ν��
			u32 add_table_filter(TableStmt* table, const Expression_s& filter);
			//expr = expr and other
			u32 make_and_expression(Expression_s& expr, const Expression_s& other);
			//ѡ�����ŵ�join order
			u32 choos_best_join_order();

		private:
			Vector<TableStmt*> table_list;
			/*��������table scan��join��filter������or�������ӵ�˫��ν��*/
			Expression_s filter_after_join;
			HashMap<TableStmt*, Expression_s> table_filters;
			/*��һ��pairָ��join�����ű��ڶ���pairָ��join condition��join equal condition
			  join condition����join equal condition*/
			HashMap<JoinableTables, JoinConditions> join_info;
		};
	}
}

#endif	//SELECT_PLAN_H