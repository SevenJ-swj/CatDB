#ifndef SELECT_STMT_H
#define SELECT_STMT_H
#include "stmt.h"
#include "type.h"

namespace CatDB {
	namespace Parser {
		DECLARE(Stmt);
		DECLARE(ExprStmt);

		class SelectStmt : public Stmt
		{
		public:
			SelectStmt();
			~SelectStmt();
			StmtType stmt_type()const;
		public:
			Vector<ExprStmt_s> select_stmts;	//select����
			Vector<ExprStmt_s> from_stmts;		//from����
			ExprStmt_s where_stmt;				//where����
			Vector<ExprStmt_s> group_columns;	//groupby ��
			ExprStmt_s having_stmt;				//having����
			Vector<ExprStmt_s> order_columns;	//order by��
			ExprStmt_s order_stmt;				//�����ǽ���
			ExprStmt_s limit_offset;			//limit��ƫ��
			ExprStmt_s limit_size;				//limit����
		};
	}
}

#endif	//SELECT_STMT_H