﻿#ifndef DELETE_H
#define DELETE_H
#include "phy_operator.h"
#include "type.h"

namespace CatDB {
	namespace Storage {
		DECLARE(TableSpace);
	}
    namespace Common {
		DECLARE(Row);
	}
	namespace Sql {
		using Storage::TableSpace_s;
        using Common::Row_s;
		DECLARE(Filter);
        
		class Delete : public PhyOperator
		{
		private:
			Delete() = delete;
			Delete(const TableSpace_s& table_space);
		public:
			~Delete();
            static PhyOperator_s make_delete(const String&database,
											const String& table,
											const String& alias_table_name,
											const Filter_s& filter);
			u32 set_filter(const Filter_s& filter);
			Filter_s get_filter()const;
			//物理算子必须提供的接口
			u32 open() override;
			u32 close() override;
			u32 reset() override;
			u32 reopen(const Row_s& row) override;
			u32 get_next_row(Row_s &row) override;
			u32 type() const override;
			u32 explain_operator(u32 depth, QueryResult* result)override;
		private:
			TableSpace_s table_space;
			Filter_s filter;
		};
	}
}

#endif	//DELETE_H