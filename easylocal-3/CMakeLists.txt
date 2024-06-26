cmake_minimum_required(VERSION 3.8)

project(EasyLocal LANGUAGES CXX)

find_package(Threads REQUIRED)
find_package(Boost 1.58.0 COMPONENTS program_options REQUIRED)
find_package(TBB)

set(subdirs helpers observers runners solvers testers utils modeling)

# Generate list of all files and directories to be added to the library

# Add files from subdirectories
foreach (subdir ${subdirs})
	string(SUBSTRING ${subdir} 0 1 FIRST_LETTER)
	string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
	string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" subdir_cap "${subdir}")

  file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/include/${subdir}/*.hh)
	source_group("Header Files\\${subdir_cap}" FILES ${headers})
endforeach (subdir)

file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hh ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh)
source_group("Header Files" FILES ${headers})

add_library(EasyLocal INTERFACE)
target_include_directories(EasyLocal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(EasyLocal INTERFACE Boost::program_options Threads::Threads)

if (TBB_FOUND)
  target_compile_definitions(EasyLocal INTERFACE TBB_AVAILABLE)
  target_link_libraries(EasyLocal INTERFACE TBB::tbb) 
endif (TBB_FOUND)
target_sources(EasyLocal INTERFACE ${headers})
target_compile_features(EasyLocal INTERFACE cxx_std_17)
target_compile_options(EasyLocal INTERFACE "-Wall")


install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/easylocal
  PATTERN *
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)


