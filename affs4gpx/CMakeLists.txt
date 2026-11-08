cmake_minimum_required(VERSION 3.10)

project(affs4gpx C)

add_executable(affs4gpx
	udpreceiver.c
	channels.c
	gpxwriter.c
	main.c
)

target_include_directories(affs4gpx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)
target_link_libraries(affs4gpx m pthread)

