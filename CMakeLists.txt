project(se2013-plugin)

add_library(plugin SHARED "${CMAKE_CURRENT_LIST_DIR}/src/plugin.cpp")

# Source SDK headers
target_include_directories(plugin PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/public"
    "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/public/tier0"
    "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/public/tier1"
)

# Link SDK libraries
find_library(SDKTier0   NAMES tier0 PATHS "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/lib/public")
find_library(SDKTier1   NAMES tier1 PATHS "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/lib/public")
find_library(SDKTier2   NAMES tier2 PATHS "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/lib/public")
find_library(SDKSTD     NAMES vstdlib PATHS "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/lib/public")
find_library(SDKMath    NAMES mathlib PATHS "${CMAKE_CURRENT_LIST_DIR}/include/source-sdk-2013/mp/src/lib/public")

target_link_libraries(plugin PRIVATE ${SDKTier0} ${SDKTier1} ${SDKTier2} ${SDKSTD} ${SDKMath} legacy_stdio_definitions.lib)