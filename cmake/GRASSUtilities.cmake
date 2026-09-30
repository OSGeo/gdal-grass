# Get definition from GRASS' Platform.make file
function(get_grass_platform_var platform_file searchvar outvar)
  set(def "")
  # GRASS built with CMake installs no include/Make/Platform.make; that file is
  # an autotools-only artifact.
  if(NOT EXISTS "${platform_file}")
    return()
  endif()
  file(STRINGS "${platform_file}" lines REGEX ".* = .*")
  foreach(line IN LISTS lines)
    string(REGEX REPLACE "(.*) = .*" "\\1" var "${line}")
    string(REGEX REPLACE ".* = (.*)" "\\1" def "${line}")
    string(STRIP "${var}" var)
    string(STRIP "${def}" def)
    if("${searchvar}" STREQUAL "${var}")
      set(${outvar}
          "${def}"
          PARENT_SCOPE)
    endif()
  endforeach()
endfunction()
