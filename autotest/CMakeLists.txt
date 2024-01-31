# CMake4GDAL project is distributed under MIT license. See accompanying file LICENSE.txt.

#[[
option(AUTOTEST_DOWNLOAD_TEST_DATA "Autotest to download test data" OFF)
option(AUTOTEST_SLOW_TEST "Autotest to run slow test" OFF)
if (NOT DEFINED ENV{CTEST_PARALLEL_LEVEL})
  set(PARALLEL_OPTION "-j1")
endif ()
]]

if (Python_Interpreter_FOUND)

  if (WIN32)
    # If running GDAL as a CustomBuild Command os MSBuild, "ERROR bla:" is considered as failing the job. This is rarely
    # the intended behavior
    list(APPEND PYTHON_RUN_ENV "CPL_ERROR_SEPARATOR=\\;")
  endif ()

  # Set TEST_ENV that goes into pytest.ini

  # Set GDAL_DATA
  if(WIN32)
    file(TO_NATIVE_PATH "${PROJECT_BINARY_DIR}/data" GDAL_DATA)
  else()
    set(GDAL_DATA "${PROJECT_BINARY_DIR}/data")
  endif()
  set(TEST_ENV_ GDAL_DATA=${GDAL_DATA})

  if (GDAL_DOWNLOAD_TEST_DATA)
    list(APPEND TEST_ENV_ GDAL_DOWNLOAD_TEST_DATA=YES)
  else ()
    list(APPEND TEST_ENV_ "#GDAL_DOWNLOAD_TEST_DATA=YES")
  endif ()
  if (GDAL_SLOW_TESTS)
    list(APPEND TEST_ENV_ GDAL_RUN_SLOW_TESTS=YES)
  else ()
    list(APPEND TEST_ENV_ "#GDAL_RUN_SLOW_TESTS=YES")
  endif ()
  # Conda enable PROJ_NETWORK but this does interfere with some of our tests due to some unexpected grid being used
  list(APPEND TEST_ENV_ PROJ_NETWORK=OFF)
  list(APPEND TEST_ENV_ "GDAL_DRIVER_PATH=${PROJECT_BINARY_DIR}")

  string(REPLACE ";" "\n   " TEST_ENV "${TEST_ENV_}")

  set(AUTOTEST_LOG_FILE "${CMAKE_CURRENT_BINARY_DIR}/autotest.log")
  set(PYTEST_INI_HEADER_MESSAGE "This file was generated from ${GDAL_CMAKE_TEMPLATE_PATH}/pytest.ini.in using ${CMAKE_CURRENT_LIST_FILE}")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pytest.ini.in ${CMAKE_CURRENT_BINARY_DIR}/pytest.ini @ONLY)
  unset(PYTEST_INI_HEADER_MESSAGE)

function (copy_file_or_dir source dest)
    if (IS_DIRECTORY ${source})
      message(STATUS "Copying contents of ${source} to ${destination}")
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${source} ${destination})
    else()
      message(STATUS "Copying ${source} to ${destination}")
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${source} ${destination})
    endif()
endfunction()

function (symlink_or_copy source destination)
    file(CREATE_LINK ${source} ${destination}
      RESULT res
      SYMBOLIC)
    if (NOT res EQUAL 0)
      copy_file_or_dir(${source} ${destination})
    endif ()
endfunction ()

  symlink_or_copy(${CMAKE_CURRENT_SOURCE_DIR}/conftest.py ${CMAKE_CURRENT_BINARY_DIR}/conftest.py)

  if (NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    foreach (subdir IN ITEMS pymod) # proj_grids cpp/data)
      if (SKIP_COPYING_AUTOTEST_SUBDIRS)
        message(STATUS "Skipping copying ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
      else ()
        symlink_or_copy(${CMAKE_CURRENT_SOURCE_DIR}/${subdir} ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
      endif ()
    endforeach ()
  endif()

  foreach (tgt IN ITEMS ogr gdrivers)
    if (NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
      if (SKIP_COPYING_AUTOTEST_SUBDIRS)
        message(STATUS "Skipping copying ${CMAKE_CURRENT_SOURCE_DIR}/${tgt}")
      else ()
        symlink_or_copy(${CMAKE_CURRENT_SOURCE_DIR}/${tgt} ${CMAKE_CURRENT_BINARY_DIR}/${tgt})
      endif ()
    endif()
    add_custom_target(
      autotest_${tgt}
      COMMAND ${CMAKE_COMMAND} -E env ${PYTHON_RUN_ENV} ${Python_EXECUTABLE} -m pytest -c
              ${CMAKE_CURRENT_BINARY_DIR}/pytest.ini ${tgt}
      DEPENDS ${GDAL_LIB_TARGET_NAME} gdalapps python_binding)
    add_test(NAME autotest_${tgt} COMMAND ${Python_EXECUTABLE} -m pytest -c ${CMAKE_CURRENT_BINARY_DIR}/pytest.ini
                                          ${tgt})
    set_property(TEST autotest_${tgt} PROPERTY ENVIRONMENT "${PYTHON_RUN_ENV}")
  endforeach ()

endif ()
