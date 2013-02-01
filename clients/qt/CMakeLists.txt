cmake_minimum_required(VERSION 2.8)

#set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(libs)

set(QT_USE_QTNETWORK TRUE)
find_package(Qt REQUIRED)
include(${QT_USE_FILE})
list(APPEND libs ${QT_LIBRARIES})

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
list(APPEND libs ${PROTOBUF_LIBRARIES})

protobuf_generate_cpp(PROTO_CPP PROTO_H
    ../../src/proto/grSim_Replacement.proto
    ../../src/proto/grSim_Commands.proto
    ../../src/proto/grSim_Packet.proto
    ../../src/proto/messages_robocup_ssl_detection.proto
    ../../src/proto/messages_robocup_ssl_geometry.proto
    ../../src/proto/messages_robocup_ssl_wrapper.proto
    ../../src/proto/messages_robocup_ssl_refbox_log.proto
)


set(app client)

QT4_WRAP_CPP(CLIENT_HEADERS_MOC mainwindow.h)

add_executable(${app} MACOSX_BUNDLE
    ${PROTO_CPP}
    ${PROTO_H}
    ${CLIENT_HEADERS_MOC} 
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(${app} ${libs})

