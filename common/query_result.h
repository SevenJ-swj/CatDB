#ifndef QUERY_RESULT_H
#define QUERY_RESULT_H
#include "object.h"

namespace CatDB {
	namespace Common {
		DECLARE(Row);

		class QueryResult : public Object
		{
		public:
			QueryResult();
			static Object_s make_query_result();
			u32 size()const;
			void add_row(const Row_s& row);
			u32 get_row(u32 idx, Row_s& row);
			u32 set_error_code(u32 code);
			u32 get_error_code()const;

			u32 serialization(u8*& buffer) override;
			bool is_fixed_length() override;
			bool bool_value() override;
			u32 hash() override;
			String to_string()const override;

		private:
			Vector<Row_s> list;
			u32 err_code;
		};
	}
}

#endif	//QUERY_RESULT_H