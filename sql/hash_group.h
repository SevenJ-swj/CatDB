#ifndef HASH_GROUP_H
#define HASH_GROUP_H

#include "phy_operator.h"
#include "hash_table.h"
#include "type.h"

namespace CatDB {
	namespace Common {
		DECLARE(Row);
		DECLARE(Object);
	}
	namespace Sql {
		using Common::Row_s;
		using Common::Object_s;
		DECLARE(Filter);
		DECLARE(Expression);

		class HashGroup : public SingleChildPhyOperator
		{
		private:
			HashGroup() = delete;
			HashGroup(PhyOperator_s& child);
		public:
			~HashGroup();
			static PhyOperator_s make_hash_group(PhyOperator_s& child, 
				const Vector<Expression_s>& group_columns,
				const Expression_s& agg_expr);
			static PhyOperator_s make_hash_group(PhyOperator_s& child,
				const Vector<Expression_s>& group_columns,
				const Expression_s& agg_expr,
				const Filter_s& filter);
			u32 set_group_columns(const Vector<Expression_s>& expr);
			u32 set_agg_expr(const Expression_s& expr);
			u32 set_filter(const Filter_s& filter);

			//�������ӱ����ṩ�Ľӿ�
			u32 open();
			u32 close();
			u32 reset();
			u32 reopen(const Row_s& row);
			u32 get_next_row(Row_s &row);
			u32 type() const;

		private:
			u32 init_hash_table();
			u32 build_hash_table();
			bool euqal(const Row_s& lhs, const Row_s& rhs);
			Row_s make_row(const Object_s& result);

			Common::HashTable hash_table;
			Vector<Expression_s> group_cols;
			Expression_s agg_func_expr;
			Filter_s filter;

			u32 alias_table_id;

			//��ǰ���Ϻ�������״̬
			u32 cur_bucket_idx;
			u32 cur_bucket_pos;
			//��û�������ʱ����Ҫ���һ�н��
			bool out_when_empty_input;
		};

	}
}

#endif	//HASH_GROUP_H