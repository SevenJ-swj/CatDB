#ifndef EXPR_STMT_H
#define EXPR_STMT_H
#include "stmt.h"
#include "type.h"

namespace CatDB {
	namespace Common {
		DECLARE(Object);
	}
	namespace Parser {
		using Common::Object_s;
		DECLARE(Stmt);
		//���ʽ���
		class ExprStmt : public Stmt
		{
		public:
			enum ExprType {
				Const = 0,
				Column,
				Table,
				Aggregate,
				Query,
				List,
				Unary,
				Binary,
				Ternary
			};
			enum OperationType {
				OP_INVALID = 0,
				OP_ADD,
				OP_SUB,
				OP_MUL,
				OP_DIV,
				OP_EQ,
				OP_NE,
				OP_GE,
				OP_GT,
				OP_LE,
				OP_LT,
				OP_BETWEEN,
				OP_NOT_BETWEEN,
				OP_IS_NULL,
				OP_IS_NOT_NULL,
				OP_IN,
				OP_NOT_IN,
				OP_EXISTS,
				OP_NOT_EXISTS,
				OP_AND,
				OP_OR,
				OP_NOT,
				OP_UNION,
				OP_UNION_ALL,
				OP_INTERSECT,
				OP_EXCEPT,
				OP_MINUS,
				OP_LIKE,
				OP_NOT_LIKE
			};
			ExprStmt();
			~ExprStmt();
			StmtType stmt_type()const;
			virtual ExprType expr_stmt_type()const = 0;
		public:
			String alias_name;	//���ʽ�ı���
		};
		//�����������ʽ
		class ConstStmt : public ExprStmt
		{
		private:
			ConstStmt();
		public:
			~ConstStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_const_stmt(const Object_s& value);
		public:
			Object_s value;
		};
		/*���б��ʽ
		columnΪ*ʱ��ʾ��������У�
		table��columnͬʱΪ*ʱ��ʾfrom�б����б��������
		*/
		class ColumnStmt : public ExprStmt
		{
		private:
			ColumnStmt();
		public:
			~ColumnStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_column_stmt(const String& table, const String& column);
			static Stmt_s make_all_column_stmt();
		public:
			String table;	//������
			String column;	//����
		};
		//һ�ű���������
		class TableStmt : public ExprStmt
		{
		private:
			TableStmt();
		public:
			~TableStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_table_stmt(const String& database, const String& table_name);
			static Stmt_s make_table_stmt(const String& table_name);
		public:
			String database;
			String table_name;			//��ʵ��
		};
		//�Ӳ�ѯ���
		class QueryStmt : public ExprStmt
		{
		private:
			QueryStmt();
		public:
			~QueryStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_query_stmt();
		public:
			Stmt_s query_stmt;
		};
		//���ʽ�б�
		class ListStmt : public ExprStmt
		{
		private:
			ListStmt();
		public:
			~ListStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_list_stmt();
		public:
			Vector<Stmt_s> stmt_list;
		};
		//�ۺϺ������
		class AggrStmt : public ExprStmt
		{
		private:
			AggrStmt();
		public:
			enum AggrType {
				SUM = 0,
				AVG,
				COUNT,
				MIN,
				MAX
			};
			~AggrStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_aggr_stmt();
		public:
			Stmt_s aggr_expr;	//�ۺϺ����ڵı��ʽ
			AggrType aggr_func;		//�ۺϺ�������
			bool distinct;
		};
		//һԪ���ʽ���
		class UnaryExprStmt :public ExprStmt
		{
		private:
			UnaryExprStmt();
		public:
			~UnaryExprStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_unary_stmt();
		public:
			Stmt_s expr_stmt;
			OperationType op_type;
		};
		//��Ԫ���ʽ���
		class BinaryExprStmt :public ExprStmt
		{
		private:
			BinaryExprStmt();
		public:
			~BinaryExprStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_binary_stmt();
		public:
			Stmt_s first_expr_stmt;
			Stmt_s second_expr_stmt;
			OperationType op_type;
		};
		//��Ԫ���ʽ���
		class TernaryExprStmt :public ExprStmt
		{
		private:
			TernaryExprStmt();
		public:
			~TernaryExprStmt();
			ExprType expr_stmt_type()const;
			static Stmt_s make_ternary_stmt();
		public:
			Stmt_s first_expr_stmt;
			Stmt_s second_expr_stmt;
			Stmt_s third_expr_stmt;
			OperationType op_type;
		};
	}
}

#endif	//EXPR_STMT_H