cmake_minimum_required(VERSION 4.0)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(ORDERMATCH_SRC
    OrderMatchApp.cpp
    Market.cpp
)

add_library(ordermatchlib STATIC ${ORDERMATCH_SRC})

target_include_directories(ordermatchlib
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${QUICKFIX_INCLUDE_DIR}
)

target_link_libraries(ordermatchlib
    ${QUICKFIX_LIBRARY}
    pthread
)
