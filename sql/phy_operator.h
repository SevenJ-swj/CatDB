#ifndef PHY_OPERATOR_H
#define PHY_OPERATOR_H
#include "type.h"

namespace CatDB {
	namespace Common {
		DECLARE(Row);
	}
	namespace Sql {
		using Common::Row_s;
		DECLARE(PhyOperator);

		//����������ӿ�
		class PhyOperator
		{
		public:
			enum OperatorType {
				TABLE_SCAN = 0,
				HASH_JOIN,
				MERGE_JOIN,
				NESTED_LOOP_JOIN,
				SORT,
				TOP_N_SORT,
				HASH_DISTINCT,
				MERGE_DISTINCT,
				HASH_GROUP,
				MERGE_GROUP,
				SCALAR_GROUP,
				HASH_SET,
				MERGE_SET,
				LIMIT,
				SUBPLAN_SCAN,
				SUBPLAN_FILTER
			};
			PhyOperator();
			virtual ~PhyOperator();

			//�������������������Դ������������ȡ�
			virtual u32 open() = 0;
			//�ر�������������ͷ���Դ���ر���������ȡ�
			virtual u32 close() = 0;
			//�������ӣ��Ա����¶�ȡ��
			virtual u32 reset() = 0;
			//���´����ӣ����������桢����״̬�����ù�����
			//Ϊsubplan filter���
			virtual u32 reopen(const Row_s& row) = 0;
			/**
			* �����һ�е�����
			* @note ���´ε���get_next����closeǰ�����ص�row��Ч
			* @pre ����open()
			* @return SUCCESS��ITER_END�������
			*/
			virtual u32 get_next_row(Row_s &row) = 0;
			//��ȡ��ǰ���ӵ�����
			virtual u32 type() const = 0;
		private:
			DISALLOW_COPY_AND_ASSIGN(PhyOperator)
		};

		class SingleChildPhyOperator :public PhyOperator
		{
		public:
			SingleChildPhyOperator(const PhyOperator_s& child);
			virtual ~SingleChildPhyOperator();
		protected:
			PhyOperator_s child;
		private:
			SingleChildPhyOperator() = delete;
		};

		class DoubleChildPhyOperator :public PhyOperator
		{
		public:
			DoubleChildPhyOperator(const PhyOperator_s& left, const PhyOperator_s& right);
			virtual ~DoubleChildPhyOperator();
		protected:
			PhyOperator_s left_child, right_child;
		private:
			DoubleChildPhyOperator() = delete;
		};
	}
}

#endif	//PHY_OPERATOR_H