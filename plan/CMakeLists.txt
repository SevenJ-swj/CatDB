include_directories(
    . 
    ${PROJECT_SOURCE_DIR}/common 
    ${PROJECT_SOURCE_DIR}/storage
	${PROJECT_SOURCE_DIR}/sql
	${PROJECT_SOURCE_DIR}/parser
    )
aux_source_directory(. DIR_PLAN_SRCS)
add_library(plan ${DIR_PLAN_SRCS})
target_link_libraries(plan sql parser common storage)