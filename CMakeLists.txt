cmake_minimum_required (VERSION 3.13)

project(Iso14443aAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(ASK_PROJECT_NAME Iso14443aAskAnalyzer)
set(ASK_SOURCES
src/ask_analyzer/Iso14443aAskAnalyzer.cpp
src/ask_analyzer/Iso14443aAskAnalyzer.h
src/ask_analyzer/Iso14443aAskAnalyzerResults.cpp
src/ask_analyzer/Iso14443aAskAnalyzerResults.h
src/ask_analyzer/Iso14443aAskAnalyzerSettings.cpp
src/ask_analyzer/Iso14443aAskAnalyzerSettings.h
src/ask_analyzer/Iso14443aAskSimulationDataGenerator.cpp
src/ask_analyzer/Iso14443aAskSimulationDataGenerator.h
)

add_analyzer_plugin(${ASK_PROJECT_NAME} SOURCES ${ASK_SOURCES})

set(LOADMOD_PROJECT_NAME Iso14443aLoadmodAnalyzer)
set(LOADMOD_SOURCES
src/loadmod_analyzer/Iso14443aLoadmodAnalyzer.cpp
src/loadmod_analyzer/Iso14443aLoadmodAnalyzer.h
src/loadmod_analyzer/Iso14443aLoadmodAnalyzerResults.cpp
src/loadmod_analyzer/Iso14443aLoadmodAnalyzerResults.h
src/loadmod_analyzer/Iso14443aLoadmodAnalyzerSettings.cpp
src/loadmod_analyzer/Iso14443aLoadmodAnalyzerSettings.h
src/loadmod_analyzer/Iso14443aLoadmodSimulationDataGenerator.cpp
src/loadmod_analyzer/Iso14443aLoadmodSimulationDataGenerator.h
)

add_analyzer_plugin(${LOADMOD_PROJECT_NAME} SOURCES ${LOADMOD_SOURCES})