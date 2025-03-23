include(cmake/CPM.cmake)

cpmaddpackage(
        NAME serious-io-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-io-library.git"
        GIT_TAG main)

sl_target_link_system_libraries(${PROJECT_NAME}
        INTERFACE
        # serious libraries
        sl::io
)

sl_add_example(${PROJECT_NAME} 00_http_v1_server)
# sl_add_example(${PROJECT_NAME} 01_http_client)
