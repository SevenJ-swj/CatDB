#include "table_space.h"
#include "IoService.h"
#include "error.h"
#include "page.h"
#include "row.h"
#include "log.h"
using namespace CatDB::Storage;
using namespace CatDB::Common;

TableSpace::TableSpace(const String & table_id)
	:table_id(table_id),
	cur_page_offset(0)
{
	io = IoService::make_io_service();
}

TableSpace::~TableSpace()
{
}

TableSpace_s TableSpace::make_table_space(const String & table_id)
{
	return TableSpace_s(new TableSpace(table_id));
}

u32 TableSpace::open()
{
	Log(LOG_TRACE, "TableSpace", "open table space %s", table_id.c_str());
	return io->open(table_id);
}

u32 TableSpace::get_next_row(Row_s & row)
{
	Page_s page;
	u32 ret = get_page_from_offset(cur_page_offset, page);
	if (ret != SUCCESS){
		Log(LOG_ERR, "TableSpace", "can not get page %u, error code:%s", cur_page_offset, err_string(ret));
		return ret;
	}
	//当前页扫描完成
	if (!page->have_row()){
		Page_s last_page;
		get_last_page(last_page);
		//最后一页
		if (cur_page_offset == last_page->page_offset()){
			Log(LOG_TRACE, "TableSpace", "read end of table space");
			return END_OF_TABLE_SPACE;
		}
		Log(LOG_TRACE, "TableSpace", "page %u have read end, load next page", cur_page_offset);
		//读取下一页
		cur_page_offset = page->next_page_offset();
		//下一页不在内存中
		ret = get_page_from_offset(cur_page_offset, page);
		if (ret != SUCCESS){
			Log(LOG_ERR, "TableSpace", "get page %u failed,%s", cur_page_offset, err_string(ret));
			return ret;
		}
	}
	
	//TODO filter 放这还是下压到page层面？
	return page->get_next_row(row);
}

u32 TableSpace::reset()
{
	cur_page_offset = 0;
	for (auto iter = pages.begin(); iter != pages.end(); ++iter){
		iter->second->reset();
	}
	return SUCCESS;
}

u32 TableSpace::close()
{
	for (auto iter = pages.begin(); iter != pages.end(); ++iter){
		iter->second->close();
		pages_copy.push_back(iter->second);
	}
	pages.clear();
	pages_copy.clear();
	Log(LOG_TRACE, "TableSpace", "close table space %s", table_id.c_str());
	return SUCCESS;
}

u32 TableSpace::insert_row(const Row_s & row)
{
	u32 row_id;
	Page_s page;
	get_last_page(page);
	//当前页能够存放下行
	if (page->have_free_space_insert(row)){
		return page->insert_row(row_id, row);
	}else{//创建新的页存放
		Log(LOG_TRACE, "TableSpace", "page %u have no free space to insert row", page->page_offset());
		u32 offset = page->next_page_offset();
		Page_s page;
		create_page(offset, page);
		return page->insert_row(row_id, row);
	}
}

u32 TableSpace::update_row(const Row_s & row)
{
	u32 row_id = row->get_row_id();
	Page_s page;
	u32 ret = get_page_from_row_id(row_id, page);
	if (ret == SUCCESS){
		return page->update_row(row_id, row);
	}else{
		Log(LOG_ERR, "TableSpace", "can not get row %u `s page,%s", row_id, err_string(ret));
		return ret;
	}
}

u32 TableSpace::delete_row(u32 row_id)
{
	Page_s page;
	u32 ret = get_page_from_row_id(row_id, page);
	if (ret == SUCCESS){
		return page->delete_row(row_id);
	}else{
		Log(LOG_ERR, "TableSpace", "can not delete row %u,%s", row_id, err_string(ret));
		return ret;
	}
}

u32 TableSpace::get_table_id() const
{
	Hash<String> hash;
	return hash(table_id);
}

u32 TableSpace::get_page_from_row_id(u32 row_id, Page_s& page)
{
	u32 page_offset = get_page_offset_from_row_id(row_id);
	if (pages.find(page_offset) == pages.end()){
		Log(LOG_TRACE, "TableSpace", "page %u is not in memory", page_offset);
		u32 ret = read_page(page_offset, page);
		return ret;
	}else{
		page = pages[page_offset];
		return SUCCESS;
	}
}

u32 TableSpace::get_page_from_offset(u32 page_offset, Page_s & page)
{
	if (pages.find(page_offset) == pages.end()){
		Log(LOG_TRACE, "TableSpace", "page %u is not in memory", page_offset);
		u32 ret = read_page(page_offset, page);
		return ret;
	}else{
		page = pages[page_offset];
		return SUCCESS;
	}
}

u32 TableSpace::read_page(u32 page_offset, Page_s & page)
{
	u32 offset = 0;
	u32 ret = io->end_offset(offset);
	if (ret == EMPTY_TABLE_SPACE || offset < page_offset){
		Log(LOG_INFO, "TableSpace", "page %u not in table space", page_offset);
		return END_OF_TABLE_SPACE;
	}
	page = Page::make_page(io, get_table_id(), page_offset, 0, 0, page_offset);
	ret = io->read_page(page);
	if (ret != SUCCESS){
		return ret;
	}else{
		ret = page->open();
		if (ret != SUCCESS){
			Log(LOG_ERR, "TableSpace", "open page %u failed,%s", cur_page_offset, err_string(ret));
			return ret;
		}
		pages[page_offset] = page;
		return ret;
	}
}

u32 TableSpace::create_page(u32 page_offset, Page_s & page)
{
	u32 page_pre, page_next;
	if (page_offset == 0)
		page_pre = 0;
	else
		page_pre = page_offset - PAGE_SIZE;
	page_next = page_offset + PAGE_SIZE;
	u32 beg_row_id = get_beg_row_id_from_page_offset(page_offset);
	page = Page::make_page(io, get_table_id(), page_offset, page_pre, page_next, beg_row_id);
	pages[page_offset] = page;
	return SUCCESS;
}

u32 TableSpace::get_last_page(Page_s & page)
{
	u32 offset = 0;
	u32 ret = io->end_offset(offset);
	//空表
	if (ret == EMPTY_TABLE_SPACE){
		//内存中也没有数据页
		if (pages.empty()){
			create_page(offset, page);
			return SUCCESS;
		}else{//否则内存中最后一页
			auto iter = pages.end();
			--iter;
			page = iter->second;
			return SUCCESS;
		}
	}else{
		//获取磁盘的最后一页
		u32 ret = get_page_from_offset(offset, page);
		if (ret != SUCCESS){
			return ret;
		}
		auto iter = pages.end();
		--iter;
		//内存最后一页在磁盘最后一页之后
		if (iter->first > offset)
			page = iter->second;
		return SUCCESS;
	}
}