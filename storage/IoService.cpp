#include <stdio.h>
#include "IoService.h"
#include "buffer.h"
#include "error.h"
#include "page.h"
#include "log.h"

using namespace CatDB::Storage;

IoService::IoService()
	:file_handle(0)
{
	
}

IoService::~IoService()
{
	if (file_handle){
		fclose(file_handle);
	}
}

IoService_s IoService::make_io_service()
{
	return IoService_s(new IoService());
}

u32 IoService::open(const String & table_file)
{
	Log(LOG_INFO, "IoService", "open table %s", table_file.c_str());
	file_handle = fopen(table_file.c_str(), "r+b");
	if (file_handle){
		return SUCCESS;
	}else{
		file_handle = fopen(table_file.c_str(), "w");
		if (file_handle){
			fclose(file_handle);
			file_handle = fopen(table_file.c_str(), "r+b");
			return SUCCESS;
		}else{
			Log(LOG_ERR, "IoService", "open table file failed!");
			return TABLE_FILE_NOT_EXISTS;
		}
	}
}

bool CatDB::Storage::IoService::is_open() const
{
	return file_handle != 0;
}

u32 IoService::read_page(Page_s & page)
{
	u32 offset = page->page_offset();
	u32 size = page->page_size();
	const Buffer_s& buffer = page->page_buffer();
	//如果在文件已经读取到结尾时，fstream的对象会将内部的eof state置位，
	//这时使用seekg（）函数不能将该状态去除，需要使用clear（）
	
	if (fseek(file_handle, offset, SEEK_SET) != 0){
		Log(LOG_ERR, "IoService", "set offset error when read page!");
		return UNKNOWN_PAGE_OFFSET;
	}
	u32 ret = fread(buffer->buf, 1, size, file_handle);
	if (ret != size){
		Log(LOG_ERR, "IoService", "read page error:%u", ret);
		return BAD_PAGE_IN_FILE;
	}
	page->reset_page();
	Log(LOG_TRACE, "IoService", "read page %u, size %u", offset, size);
	return SUCCESS;
}

u32 IoService::write_page(const Page_s & page)
{
	return write_page(page.get());
}

u32 IoService::write_page(const Page * page)
{
	u32 offset = page->page_offset();
	u32 size = page->page_size();
	const Buffer_s& buffer = page->page_buffer();

	if (fseek(file_handle, offset, SEEK_SET) != 0){
		Log(LOG_ERR, "IoService", "set offset error when write page!");
		return UNKNOWN_PAGE_OFFSET;
	}
	u32 ret = fwrite(buffer->buf, 1, size, file_handle);
	if (ret != size){
		Log(LOG_ERR, "IoService", "write page error:%u", ret);
		return WRITE_PAGE_ERROR;
	}
	Log(LOG_TRACE, "IoService", "read page %u, size %u", offset, size);
	return SUCCESS;
}

u32 IoService::end_offset(u32& offset)
{
	fseek(file_handle, 0, SEEK_END);
	u32 size = ftell(file_handle);
	if (size >= PAGE_SIZE){
		offset = size - PAGE_SIZE;
		return SUCCESS;
	}else{
		return EMPTY_TABLE_SPACE;
	}
}

void IoService::close()
{
	fclose(file_handle);
}

bool IoService::eof() const
{
	return feof(file_handle) != 0;
}