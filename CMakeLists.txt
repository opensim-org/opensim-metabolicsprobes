project(OpenSimMuscleMetabolicsProbes)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
find_package(OpenSim REQUIRED)

include_directories(${OPENSIMSIMBODY_INCLUDE_DIRS})

set(SOURCE
    UchidaBhargava2004MuscleMetabolicsProbe.h
    UchidaBhargava2004MuscleMetabolicsProbe.cpp
    UchidaUmberger2010MuscleMetabolicsProbe.h
    UchidaUmberger2010MuscleMetabolicsProbe.cpp
    RegisterTypes_osimMuscleMetabolicsProbes.h
    RegisterTypes_osimMuscleMetabolicsProbes.cpp
    osimMuscleMetabolicsProbesDLL.h
    )

add_library(osimMuscleMetabolicsProbes SHARED ${SOURCE})
set_target_properties(osimMuscleMetabolicsProbes PROPERTIES
	DEFINE_SYMBOL OSIMMUSCLEMETABOLICSPROBES_EXPORTS)
target_link_libraries(osimMuscleMetabolicsProbes ${OPENSIMSIMBODY_LIBRARIES})

install(TARGETS osimMuscleMetabolicsProbes DESTINATION .)
install(FILES README.txt DESTINATION .)
install(DIRECTORY examples DESTINATION .)

enable_testing()
include(CTest)

include_directories(${PROJECT_SOURCE_DIR})
add_executable(testMuscleMetabolicsProbes tests/testMuscleMetabolicsProbes.cpp)
target_link_libraries(testMuscleMetabolicsProbes ${OPENSIMSIMBODY_LIBRARIES}
    osimMuscleMetabolicsProbes)
add_test(testMuscleMetabolicsProbes testMuscleMetabolicsProbes)

