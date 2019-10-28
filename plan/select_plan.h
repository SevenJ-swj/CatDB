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
			void set_alias_table_id(u32 id);
			u32 get_alias_table_id()const;
		private:
			/*��һ��pairָ��join�����ű��ڶ���pairָ��join condition��join equal condition
			  join condition����join equal condition*/
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
			//����select list�е�*��
			u32 resolve_all_column_in_select_list(const Stmt_s& stmt);
			//����count�ۺϺ����ڵ�*���ʽ
			u32 resolve_all_column_in_count_agg(const Stmt_s& stmt, Expression_s& expr);
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
			u32 search_jon_info(const JoinableTables& join_tables, JoinConditions*& join_cond);
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
			//����group��
			u32 resolve_group_stmt(const Stmt_s& group_stmt);
			//����having�Ӿ�
			u32 resolve_having_stmt(const Stmt_s& having_stmt);
			//����limit�Ӿ�
			u32 resolve_limit_stmt(const Stmt_s& limit_stmt);
			//����sort��
			u32 resolve_sort_stmt(const Stmt_s& sort_stmt);
			u32 resolve_select_list(const Stmt_s& select_list);
			u32 add_access_column(TableStmt* table, const ColumnDesc& col_desc);
			u32 make_access_row_desc();
			u32 make_join_plan(PhyOperator_s& op);
			u32 make_table_scan(TableStmt* table, PhyOperator_s& op);
			u32 make_group_pan(PhyOperator_s& op);
			u32 make_sort_plan(PhyOperator_s& op);
			u32 make_query_plan(PhyOperator_s& op);
			u32 make_limit_and_distinct_plan(PhyOperator_s& op);
		private:
			//select list�����г��ֹ��ľۺϺ���
			Vector<Expression_s> aggr_exprs;
			//select list������ÿһ��������ʽ���ۺϺ���Ԥ����������select_list��������
			Vector<Expression_s> select_list;
			//from list�ı�
			Vector<TableStmt*> table_list;
			/*��������table scan��join��filter������or�������ӵ�˫��ν��*/
			Expression_s filter_after_join;
			//ÿ�ű�Ĺ���ν��
			HashMap<TableStmt*, Expression_s> table_filters;
			//ÿ�ű���Ҫ���ʵ���
			HashMap<TableStmt*, Vector<ColumnDesc> > table_access_column;
			HashMap<TableStmt*, RowDesc > table_access_row_desc;
			//��������������Ϣ
			HashMap<JoinableTables, JoinConditions> join_info;
			//�ۺ���
			Vector<Expression_s> group_cols;
			//having����ν�ʱ��ʽ
			Expression_s having_filter;
			//������
			Vector<Expression_s> sort_cols;
			//limit���
			u32 limit_offset, limit_size;
			//��ǰ�ڽ���select_list����having���飬�������־ۺϺ����Ľ�������
			u32 resolve_select_list_or_having;
			//��ѯ���ɵ���ʱ��ID
			u32 alias_table_id;
			bool is_distinct;
			bool asc;
			bool have_limit;
		};
	}
}

#endif	//SELECT_PLAN_H