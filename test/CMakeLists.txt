add_library(esp_logger_core STATIC
    ${PROJECT_SOURCE_DIR}/src/esp_logger/logger.cpp
)

target_include_directories(esp_logger_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

target_compile_features(esp_logger_core PUBLIC cxx_std_17)

add_library(esp_logger_core_json STATIC
    ${PROJECT_SOURCE_DIR}/src/esp_logger/logger.cpp
)

target_include_directories(esp_logger_core_json
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs_json
)

target_compile_features(esp_logger_core_json PUBLIC cxx_std_17)

add_executable(logger_tests
    logger_tests.cpp
    logger_test_stubs.cpp
)

target_include_directories(logger_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

target_link_libraries(logger_tests
    PRIVATE
        esp_logger_core
)

target_compile_features(logger_tests PRIVATE cxx_std_17)

add_test(NAME logger_tests COMMAND logger_tests)

add_executable(logger_json_tests
    logger_json_tests.cpp
    logger_test_stubs.cpp
)

target_include_directories(logger_json_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs_json
)

target_link_libraries(logger_json_tests
    PRIVATE
        esp_logger_core_json
)

target_compile_features(logger_json_tests PRIVATE cxx_std_17)

add_test(NAME logger_json_tests COMMAND logger_json_tests)
