# Mac Plist Macros

function(get_version_plist plistfile outvar)
  set(pversion "")
  if(EXISTS ${plistfile})
    file(READ "${plistfile}" info_plist)
    string(REGEX REPLACE "\n" "" info_plist "${info_plist}")
    string(
      REGEX
        MATCH
        "<key>CFBundleShortVersionString</key>[ \t]*<string>([0-9\\.]*)</string>"
        PLISTVERSION
        "${info_plist}")
    string(
      REGEX
      REPLACE
        "<key>CFBundleShortVersionString</key>[ \t]*<string>([0-9\\.]*)</string>"
        "\\1"
        pversion
        "${PLISTVERSION}")
  endif(EXISTS ${plistfile})
  set(${outvar}
      ${pversion}
      PARENT_SCOPE)
endfunction(get_version_plist)
