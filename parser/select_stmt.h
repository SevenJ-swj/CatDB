#ifndef SELECT_STMT_H
#define SELECT_STMT_H
#include "stmt.h"
#include "type.h"

namespace CatDB {
	namespace Parser {
		DECLARE(Stmt);

		class SelectStmt : public Stmt
		{
		private:
			SelectStmt();
		public:
			~SelectStmt();
			StmtType stmt_type()const;
			static Stmt_s make_select_stmt();
		public:
			Stmt_s select_expr_list;	//select����
			bool is_distinct;				//������Ƿ�ȥ�أ�
			Stmt_s from_stmts;		//from����
			Stmt_s where_stmt;				//where����
			Stmt_s group_columns;	//groupby ��
			Stmt_s having_stmt;				//having����
			Stmt_s order_columns;	//order by��
			bool asc_desc;					//�����ǽ���,trueΪasc��falseΪdesc��Ĭ��asc
			Stmt_s limit_stmt;
		};

		class LimitStmt : public Stmt
		{
		private:
			LimitStmt();
		public:
			~LimitStmt();
			StmtType stmt_type()const;
			static Stmt_s make_limit_stmt(u32 size, u32 offset = 0);
		private:
			u32 limit_offset;			//limit��ƫ��
			u32 limit_size;				//limit����
		};
	}
}

#endif	//SELECT_STMT_H