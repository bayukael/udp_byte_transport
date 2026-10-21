cmake_minimum_required(VERSION 3.13)
project(pendarlab_lib_comm_udp_byte_transport VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(NAMESPACE pendarlab)
set(PACKAGE_FILENAME udp_byte_transport)
set(PACKAGE_NAME PendarlabUdpByteTransport)

find_package(PendarlabByteTransport 1.0.0 REQUIRED)
find_package(GTest CONFIG REQUIRED)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/udp_byte_transport/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/udp_byte_transport/Version.h
    @ONLY
)

add_library(UdpByteTransport SHARED
  src/UdpByteTransport.cpp
  src/UdpByteTransportDefinition.cpp
)
target_include_directories(UdpByteTransport PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:include> 
)
target_link_libraries(UdpByteTransport 
  PUBLIC pendarlab::ByteTransport
)
add_library(${NAMESPACE}::UdpByteTransport ALIAS UdpByteTransport)
set_target_properties(UdpByteTransport PROPERTIES
    OUTPUT_NAME pendarlab-udp_byte_transport
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)


##############################################################################
######################     Define Files to Install     #######################
##############################################################################

set(TARGET_EXPORT_NAME UdpByteTransportTargets)
set(TARGET_EXPORT_FILENAME udp_byte_transport_targets)
set(PACKAGE_DIRECTORY_DESTINATION lib/cmake/${PACKAGE_NAME})

install(
	TARGETS UdpByteTransport
	EXPORT ${TARGET_EXPORT_NAME}
)
install(
	DIRECTORY include/
	DESTINATION include
	PATTERN "*.in" EXCLUDE
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/include/udp_byte_transport/Version.h
	DESTINATION include/udp_byte_transport
)
install(
	EXPORT ${TARGET_EXPORT_NAME}
	FILE ${TARGET_EXPORT_FILENAME}.cmake
	NAMESPACE ${NAMESPACE}::
	DESTINATION ${PACKAGE_DIRECTORY_DESTINATION}
)

##############################################################################
##########################     CMake Packaging     ###########################
##############################################################################

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PACKAGE_FILENAME}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake
    INSTALL_DESTINATION ${PACKAGE_DIRECTORY_DESTINATION}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}ConfigVersion.cmake
    DESTINATION ${PACKAGE_DIRECTORY_DESTINATION}
)

##################################################################################################################
################################################  BUILDING TEST  #################################################
##################################################################################################################
add_executable(udp_byte_transport_config_test test/udp_byte_transport_config_test.cpp)
target_include_directories(udp_byte_transport_config_test PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(udp_byte_transport_config_test
  PRIVATE UdpByteTransport
  PRIVATE gtest
)

add_executable(udp_byte_transport_test test/udp_byte_transport_test.cpp)
target_include_directories(udp_byte_transport_test PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(udp_byte_transport_test
  PRIVATE UdpByteTransport
  PRIVATE gtest
)

add_executable(udp_byte_transport_integration_test test/udp_byte_transport_integration_test.cpp)
target_include_directories(udp_byte_transport_integration_test PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(udp_byte_transport_integration_test
  PRIVATE UdpByteTransport
  PRIVATE gtest
)
