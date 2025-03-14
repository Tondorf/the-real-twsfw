if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/twsfw-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package twsfw)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT twsfw_Development
)

install(
    TARGETS twsfw_twsfw
    EXPORT twsfwTargets
    RUNTIME #
    COMPONENT twsfw_Runtime
    LIBRARY #
    COMPONENT twsfw_Runtime
    NAMELINK_COMPONENT twsfw_Development
    ARCHIVE #
    COMPONENT twsfw_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    twsfw_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE twsfw_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(twsfw_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${twsfw_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT twsfw_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${twsfw_INSTALL_CMAKEDIR}"
    COMPONENT twsfw_Development
)

install(
    EXPORT twsfwTargets
    NAMESPACE twsfw::
    DESTINATION "${twsfw_INSTALL_CMAKEDIR}"
    COMPONENT twsfw_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
