#ifndef IO_SERVICE_H
#define IO_SERVICE_H
#include "type.h"

namespace CatDB {
	namespace Storage {
		DECLARE(Page);
		DECLARE(IoService);

		class IoService
		{
		private:
			IoService();
		public:
			~IoService();
			static IoService_s make_io_service();
			u32 open(const String& table_file);
			bool is_open()const;
			u32 read_page(Page_s& page);
			u32 write_page(const Page_s& page);
			u32 write_page(const Page* page);
			u32 end_offset(u32& offset);
			void close();
			bool eof()const;
			u32 delete_file(const String& table_file);
			u32 clear_file(const String& table_file);

		private:
			FILE* file_handle;
		private:
			DISALLOW_COPY_AND_ASSIGN(IoService)
		};
	}
}

#endif	//IO_SERVICE_H
