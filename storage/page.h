#ifndef PAGE_H
#define PAGE_H
#include "type.h"
//PAGE_SIZE��row_id�Ľṹ�����أ��Ĵ��ע���Ƿ���ǰ����row_id
//���row_id���
#define PAGE_SIZE 16384

namespace CatDB {
	namespace Common {
		DECLARE(Buffer);
		DECLARE(Row);
	}
	namespace Storage {
		DECLARE(Page);
		DECLARE(IoService);
		using Common::Buffer_s;
		using Common::Row_s;

		//page���ļ���Ϣ
		struct FileHeader
		{
			u32 page_checksum;	//У���
			u32 table_id;		//������ռ�
			u32 page_offset;	//��ռ��ڵ�ƫ��
			u32 page_pre;		//��һҳ
			u32 page_next;		//��һҳ
		};
		/* row_id�Ķ��壺ǰ22λΪ��ǰҳƫ�Ƶ������ַ��
		 * ҳʵ��ƫ�Ƶ��������ַ����14λ
		 * ��10λΪҳ��row_id��0x0 ~ 0x3ff
		 * ����page_offset�ǵ�ַ������row_id�����ǵ�����Ψһ�ģ�
		 * ͬʱ���Ը���row_idֱ�Ӷ�λ��page��λ��*/
		u32 get_page_offset_from_row_id(u32 row_id);
		u32 get_beg_row_id_from_page_offset(u32 page_offset);

		struct PageHeader
		{
			u32 beg_row_id;		//��ż�¼����ʼ��
			u32 end_row_id;		//��ż�¼�Ľ�����
			u32 row_count;		//��ŵ�����
			u32 free_offset;	//���пռ�ƫ��
			u32 free_size;		//���пռ��С
		};

		struct RowInfo
		{
			u32 row_id;
			u32 offset;
		};

		class RawRecord
		{
		public:
			u32 column_count;	//������
			u32 column_offset[1];	//ÿ���������ڵ�ƫ��
			u32 size()const;
			static RawRecord* make_raw_record(void* ptr);
		};

		class Page
		{
		public:
			~Page();
			static Page_s make_page(
				IoService_s& io_service,
				u32 table_id,
				u32 page_offset,
				u32 page_pre,
				u32 page_next,
				u32 beg_row_id);
			//���ڿ���ɨ������������
			u32 open();
			u32 get_next_row(Row_s& row);
			bool have_row()const;
			u32 reset();
			u32 close();

			u32 get_free_space()const;
			u32 select_row(u32 row_id, Row_s& row)const;
			u32 insert_row(u32& row_id, const Row_s& row);
			u32 update_row(u32 row_id, const Row_s& row);
			u32 delete_row(u32 row_id);

			u32 page_size()const;
			u32 page_offset()const;
			u32 next_page_offset()const;
			const Buffer_s& page_buffer()const;
			void reset_page();
			u32 row_id_exists(u32 row_id)const;
			bool have_free_space_insert(const Row_s& row);

		private:
			Page(const Buffer_s& buffer, IoService_s& io_service);
			u32 project_all_column(RawRecord* record, Row_s& row)const;
			u32 project_row(RowInfo* row_info, Row_s& row)const;
			u32 write_row(RawRecord* record, const Row_s& row);
			u32 search_row(u32 row_id, RowInfo*& info)const;
			u32 set_row_id_deleted(u32& row_id);
			bool row_id_deleted(u32 row_id)const;
			u32 row_width(const Row_s& row)const;

			Buffer_s		buffer_;		//���������ڴ�
			FileHeader* file_header_;	//�ļ���Ϣ
			PageHeader* page_header_;	//ҳ��Ϣ
			u8* records_space_;	//��¼������ʼ��ַ
			u8* free_space_;	//���пռ���ʼ��ַ
			RowInfo* row_info_;		//ÿ����¼����Ϣ

			//���ڿ���ɨ������������
			u32 row_idx_;
			//��֤������д�ر�ռ�
			IoService_s io_service_;
			bool is_dirty;

		private:
			DISALLOW_COPY_AND_ASSIGN(Page)
		};
	}
}
#endif //PAGE_H