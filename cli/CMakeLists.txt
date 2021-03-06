cmake_minimum_required(VERSION 2.8.2)

set(CMAKE_COLOR_MAKEFILE ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

project(vata)

set(vata_compiler_add_flags_list
  -std=c++0x
  -pedantic-errors
  -Wextra
  -Wall
  -Wfloat-equal
  -Wctor-dtor-privacy
  -Weffc++
  -Woverloaded-virtual
  -Wold-style-cast
  -fdiagnostics-show-option
  -march=native
)

foreach(param ${vata_compiler_add_flags_list})
  set(vata_compiler_flags "${vata_compiler_flags} ${param}")
endforeach(param)

# uncomment for profiling
#add_definitions(-pg)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")

include_directories(../include)

add_executable(vata
  vata.cc
  parse_args.cc
)

get_target_property(vata_sources vata SOURCES)

foreach(src ${vata_sources})

  set_source_files_properties(
    ${src} PROPERTIES COMPILE_FLAGS ${vata_compiler_flags})

endforeach()

target_link_libraries(vata libvata)
target_link_libraries(vata rt)
