CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
project(ssb-spoofer CXX)

set(SPOOFER_SRC_DIR src)
set(SPOOFER_HDR_DIR hdr)

# ---- Fetch rt-recon-sdk from GitHub using FetchContent ----
include(FetchContent)

# Option: set to a specific commit hash, tag, or branch
# Using a commit hash is recommended for reproducibility
set(RT_RECON_SDK_COMMIT "main" CACHE STRING "rt-recon-sdk git ref (commit hash, tag, or branch)")

FetchContent_Declare(
    rt-recon-sdk
    GIT_REPOSITORY https://github.com/cueltschey/rt-recon-sdk.git
    GIT_TAG        ${RT_RECON_SDK_COMMIT}
    GIT_SHALLOW   TRUE
)

message(STATUS "Fetching rt-recon-sdk from GitHub (ref: ${RT_RECON_SDK_COMMIT})...")
FetchContent_MakeAvailable(rt-recon-sdk)

# Check if the library target exists
if(NOT TARGET rt-recon-autoconfig)
    message(FATAL_ERROR "rt-recon-sdk fetched but rt-recon-autoconfig target not found. Check the repository structure.")
endif()

get_target_property(RT_RECON_SDK_INCLUDE_DIR rt-recon-autoconfig INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "RT_RECON_SDK_INCLUDE_DIR: ${RT_RECON_SDK_INCLUDE_DIR}")

# Common source files (shared between executables)
set(COMMON_SOURCES
    "${SPOOFER_SRC_DIR}/config.cc"
    "${SPOOFER_SRC_DIR}/rf_handler.cc"
    "${SPOOFER_SRC_DIR}/ssb_processor.cc"
    "${SPOOFER_SRC_DIR}/app_core.cpp"
)

# Main SSB Spoofer executable
add_executable(ssb_spoofer 
    ${COMMON_SOURCES}
    "${SPOOFER_SRC_DIR}/main.cpp"
)

set_target_properties(ssb_spoofer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)

target_include_directories(ssb_spoofer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${SPOOFER_HDR_DIR}"
    ${RT_RECON_SDK_INCLUDE_DIR}
)

target_link_libraries(ssb_spoofer PRIVATE
    rt-recon-autoconfig
    srsran_phch
    srsran_phy
    srsran_common
    support
    srslog
    srsran_rf
    pthread
    yaml-cpp
)

# SSB File Analyzer executable
add_executable(ssb_file_analyzer
    "${SPOOFER_SRC_DIR}/ssb_processor.cc"
    "${SPOOFER_SRC_DIR}/ssb_file_analyzer.cpp"
)

set_target_properties(ssb_file_analyzer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)

target_include_directories(ssb_file_analyzer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${SPOOFER_HDR_DIR}"
    ${RT_RECON_SDK_INCLUDE_DIR}
)

target_link_libraries(ssb_file_analyzer PRIVATE
    rt-recon-autoconfig
    srsran_phch
    srsran_phy
    srsran_common
    support
    srslog
    pthread
    yaml-cpp
)

install(TARGETS ssb_spoofer ssb_file_analyzer DESTINATION /usr/local/bin OPTIONAL)
