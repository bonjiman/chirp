# Install rules and the chirp CMake package, so that other projects can use
# an installed chirp with:
#
#     find_package(chirp 0.1 REQUIRED)
#     target_link_libraries(app PRIVATE chirp::chirp)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CHIRP_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/chirp)

install(TARGETS chirp
    EXPORT chirpTargets
    FILE_SET HEADERS
)
install(EXPORT chirpTargets
    NAMESPACE chirp::
    DESTINATION ${CHIRP_INSTALL_CMAKEDIR}
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/chirpConfig.cmake.in
    ${PROJECT_BINARY_DIR}/chirpConfig.cmake
    INSTALL_DESTINATION ${CHIRP_INSTALL_CMAKEDIR}
)
# Before 1.0, a minor release may break the API.
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/chirpConfigVersion.cmake
    COMPATIBILITY SameMinorVersion
)
install(FILES
    ${PROJECT_BINARY_DIR}/chirpConfig.cmake
    ${PROJECT_BINARY_DIR}/chirpConfigVersion.cmake
    DESTINATION ${CHIRP_INSTALL_CMAKEDIR}
)
