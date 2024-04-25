cmake_minimum_required (VERSION 3.13)

project(Iso14443aAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/Iso14443aAnalyzer.cpp
src/Iso14443aAnalyzer.h
src/Iso14443aAnalyzerResults.cpp
src/Iso14443aAnalyzerResults.h
src/Iso14443aAnalyzerSettings.cpp
src/Iso14443aAnalyzerSettings.h
src/Iso14443aSimulationDataGenerator.cpp
src/Iso14443aSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
