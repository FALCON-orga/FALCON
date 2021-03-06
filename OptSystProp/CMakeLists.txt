# library name
set(LIBNAME "coffeeOptSystProp")
set(SRCNAME "OptSystProp")

set(SOURCEFILES
	${SRCNAME}.c
	${SRCNAME}.h
	OptSystProp_propagateCube.c
	OptSystProp_run.c
)

set(INCLUDEFILES
	${SRCNAME}.h
)

set(SCRIPTS
)

set(LINKLIBS
	CLIcore
	milkWFpropagate
)


# DEFAULT SETTINGS
# Do not change unless needed
# =====================================================================

project(lib_${LIBNAME}_project)

include_directories ("${PROJECT_SOURCE_DIR}/src")
include_directories ("${PROJECT_SOURCE_DIR}/..")


# Library can be compiled from multiple source files
# Convention: the main souce file is named <libname>.c
#
add_library(${LIBNAME} SHARED ${SOURCEFILES})

target_include_directories(${LIBNAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${LIBNAME} PRIVATE ${LINKLIBS})

install(TARGETS ${LIBNAME} DESTINATION lib)
install(FILES ${INCLUDEFILES} DESTINATION include/${SRCNAME})

install(PROGRAMS ${SCRIPTS} DESTINATION bin)
