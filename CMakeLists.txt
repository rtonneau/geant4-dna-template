# Indicate miminum version and what error to raise if not satisfied
cmake_minimum_required(VERSION 3.16...3.27)

# Name the project
project({{PROJECT_NAME}})

set(CMAKE_CXX_STANDARD 20)

# Look for geant4 paths (normaly sets by sourcing: ~/software/geant4/geant4-v*.*.*-install/share/Geant4/geant4make/geant4make.sh
find_package(Geant4 REQUIRED ui_all vis_all)
find_package(HDF5 REQUIRED COMPONENTS C CXX) # <-- must request CXX

#Look for toml++ library (installed by vcpkg)
#find_package(tomlplusplus CONFIG REQUIRED)

include(${Geant4_USE_FILE})

# Tell CMake to take all cc/hh files recursively (modular layout support)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/header/*.hh)

# Build include path list from every header directory so existing short includes
# (e.g. "RunAction.hh") keep working while files are moved into subfolders.
set(project_include_dirs ${PROJECT_SOURCE_DIR}/header)
foreach(header_file IN LISTS headers)
    get_filename_component(header_dir ${header_file} DIRECTORY)
    list(APPEND project_include_dirs ${header_dir})
endforeach()
list(REMOVE_DUPLICATES project_include_dirs)

# Tell CMake the name of the executable to produce.
# sim.cc will be the main source code.
add_executable(sim sim.cc ${sources} ${headers})

# Still need to link all Geant4 lib
target_link_libraries(sim ${Geant4_LIBRARIES} hdf5::hdf5-shared
    hdf5::hdf5_cpp-shared)
target_include_directories(sim PRIVATE ${project_include_dirs})

add_custom_target(Simulation DEPENDS sim)

# Standalone unit test for ArgParser (no Geant4 runtime init required).
enable_testing()
add_executable(ArgParserTest test/ArgParserTest.cc src/ArgParser.cc)
target_link_libraries(ArgParserTest ${Geant4_LIBRARIES})
target_include_directories(ArgParserTest PRIVATE ${project_include_dirs})
add_test(NAME ArgParserTest COMMAND ArgParserTest)

# Standalone unit test for OutputDir (no Geant4 runtime init required).
add_executable(OutputDirTest test/OutputDirTest.cc src/OutputDir.cc)
target_link_libraries(OutputDirTest ${Geant4_LIBRARIES})
target_include_directories(OutputDirTest PRIVATE ${project_include_dirs})
add_test(NAME OutputDirTest COMMAND OutputDirTest)

# Standalone unit test for ReactionTableDump::FormatReactionLabel (pure string
# logic, no Geant4 runtime init required).
add_executable(ReactionTableDumpTest test/ReactionTableDumpTest.cc src/ReactionTableDump.cc src/DnaLogger.cc)
target_link_libraries(ReactionTableDumpTest ${Geant4_LIBRARIES})
target_include_directories(ReactionTableDumpTest PRIVATE ${project_include_dirs})
add_test(NAME ReactionTableDumpTest COMMAND ReactionTableDumpTest)

# Standalone unit test for ReactionCounter (pure accumulation/binning logic,
# no Geant4 runtime init required).
add_executable(ReactionCounterTest test/ReactionCounterTest.cc src/ReactionCounter.cc src/OutputDir.cc)
target_link_libraries(ReactionCounterTest ${Geant4_LIBRARIES})
target_include_directories(ReactionCounterTest PRIVATE ${project_include_dirs})
add_test(NAME ReactionCounterTest COMMAND ReactionCounterTest)

# Copy macro files AFTER the target is built
# Reason: file(COPY) runs at configure time, but
# CMAKE_BUILD_TYPE is known at build time with MSVC.
add_custom_command(
    TARGET sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/macro/"
    "$<TARGET_FILE_DIR:sim>/macro"
    COMMENT "Copying macro files to output directory..."
)
