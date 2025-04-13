project(BladeDetection)
message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("PROJECT_NAME       = ${PROJECT_NAME}")

add_definitions(-DPROJECT_NAME=\"${PROJECT_NAME}\")

if (CMAKE_MODE_RELEASE)
include_directories(

)

link_directories(
    
)
else()
include_directories(

)

link_directories(

)
endif()


add_compile_options(-D_GNU_SOURCE)

# include_directories(CmdMgr)

# 手动列出源文件
set(MAIN_SOURCES
    Blades.cpp
)
# 查找OpenCV库
find_package(OpenCV REQUIRED)

file(GLOB GLOB_SOURCES ./*.cpp)


add_executable( ${PROJECT_NAME} ${MAIN_SOURCES} ${GLOB_SOURCES} ) 

target_link_libraries(${PROJECT_NAME} PRIVATE pthread rt  ${OpenCV_LIBS}) 