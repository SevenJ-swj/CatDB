#ifndef PLAN_H
#define PLAN_H
#include "type.h"

namespace CatDB {
	namespace Parser {
		DECLARE(Stmt);
	}
	namespace Common {
		DECLARE(Row);
		DECLARE(Object);
	}
	namespace Server {
		DECLARE(RequestHandle);
	}
	namespace Sql {
		DECLARE(Plan);
		DECLARE(PhyOperator);
		using Parser::Stmt_s;
		using Common::Row_s;
		using Common::Object_s;
		using Server::RequestHandle_s;

		class Plan
		{
		public:
			enum PlanType {
				INSERT = 0,
				DELETE_PLAN,
				UPDATE,
				SELECT,
				CreateTable,
				CreateDatabase,
				DropTable,
				DropDatabase,
				ShowTables,
				ShowDatabases,
				DescTable,
				UseDatabase,
				Analyze
			};
		protected:
			Plan();
		public:
			virtual ~Plan();
			static Plan_s make_plan(const Stmt_s& lex_stmt);
			void set_lex_stmt(const Stmt_s& stmt);
			Stmt_s get_lex_stmt()const;
			/*finish later
			void set_thd(const Thd_s& thd);
			Thd_s get_thd()const;
			*/
			Object_s get_result();
			Row_s get_result_title()const;
			u32 affect_rows()const;
			virtual u32 execute() = 0;
			virtual u32 build_plan() = 0;
			virtual u32 optimizer() = 0;
			virtual PlanType type() const = 0;
			void set_error_code(u32 code);
			PhyOperator_s get_root_operator();
			void set_thd(RequestHandle_s& thd);

		protected:
			//将计划输出到result中
			u32 explain_plan();

			Stmt_s lex_stmt;
			Object_s result;
			Row_s result_title;
			PhyOperator_s root_operator;
			RequestHandle_s thd;
			u32 affect_rows_;
		public:
			bool is_explain;
		};
	}
}

#endif	//PLAN_H