#ifndef SQL_REWRITE_H
#define SQL_REWRITE_H
#include "expr_stmt.h"
#include "type.h"
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
		DECLARE(DeletePlan);
		DECLARE(UpdatePlan);
		DECLARE(SelectPlan);
		DECLARE(SqlRewriter);
		using Common::Row_s;
		using Parser::Stmt_s;
		using Sql::Filter_s;
		using Sql::Expression_s;
		using Sql::DeletePlan;
		using Sql::UpdatePlan;
		using Sql::SelectPlan;
		using Parser::ExprStmt;
		using Parser::TableStmt;
		using Parser::ColumnStmt;

		class SqlRewriter
		{
		public:
			SqlRewriter(SelectPlan* parent_query, Expression_s& subquery_expr);
			~SqlRewriter();
			static SqlRewriter_s make_sql_rewriter(SelectPlan* parent_query,
				Expression_s& subquery_expr,
				const Expression_s& lhs, 
				ExprStmt::OperationType op_type);
			u32 rewrite_for_select(Expression_s& expr);
		private:
			//Ϊselect plan��дN�͵�IN��EXISTS�Ӳ�ѯ����semi join��д
			u32 rewrite_for_select_N_semi(Expression_s& expr);
			//Ϊselect plan��дN�͵�NOT IN��NOT EXISTS�Ӳ�ѯ����anti join��д
			u32 rewrite_for_select_N_anti(Expression_s& expr);
			//Ϊselect plan��дJ�͵�IN��EXISTS�Ӳ�ѯ����semi join��д
			u32 rewrite_for_select_J_semi(Expression_s& expr);
			//Ϊselect plan��дJ�͵�NOT IN��NOT EXISTS�Ӳ�ѯ����anti join��д
			u32 rewrite_for_select_J_anti(Expression_s& expr);
			//Ϊselect plan��дJA�͵Ĳ�ѯ����semi join��д
			u32 rewrite_for_select_JA_semi(Expression_s& expr);

		private:
			Expression_s subquery_expr;
			Plan_s subquery;
			SelectPlan* subquery_plan;
			SelectPlan* parent_query_plan;
			Expression_s lhs;
			ExprStmt::OperationType op_type;
			String subquery_alias;
			bool is_correlated;
		};
	}
}

#endif	//SQL_REWRITE_H