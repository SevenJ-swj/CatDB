#ifndef QUERY_H
#define QUERY_H
#include "phy_operator.h"
#include "type.h"

namespace CatDB {
	namespace Common {
		DECLARE(Row);
	}
	namespace Sql {
		using Common::Row_s;
		DECLARE(Expression);
		//���ܣ�ʹ���²����ӵ�����������û���ѯ���ʽ
		class Query : public SingleChildPhyOperator
		{
		private:
			Query() = delete;
			Query(PhyOperator_s& child, const Vector<Expression_s>& query_list);
		public:
			static PhyOperator_s make_query(PhyOperator_s& child, const Vector<Expression_s>& query_list);
			u32 add_query_item(const Expression_s& item);
			u32 set_alias_table_id(u32 id);
			u32 get_alias_table_id()const;
			//�������ӱ����ṩ�Ľӿ�
			u32 open();
			u32 close();
			u32 reset();
			u32 reopen(const Row_s& row);
			u32 get_next_row(Row_s &row);
			u32 type() const;
		private:
			Row_s make_row(const Row_s& row);

			Vector<Expression_s> query_list;
			u32 alias_table_id;
		};
	}
}

#endif	//QUERY_H
