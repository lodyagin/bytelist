file(GLOB_RECURSE TEST_SRC *.cpp)

# Enables testing
enable_testing()

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
find_package(GMock REQUIRED)
include_directories(${GMOCK_INCLUDE_DIRS})

# Locate pthreads
#find_package(Threads REQUIRED)

# convenient function to add new tests; example of adding a new test:
# addtest(my_test_name my_test_name.cpp)
# target_link_libraries(my_test_name test_essential)
function(addtest test_name test_src)
	add_executable(${test_name} ${test_src})
	target_link_libraries(
		${test_name}
		gtest ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} ${GMOCK_BOTH_LIBRARIES}
		#Threads::Threads
	)
	#target_link_libraries(${test_name} gtest_main)
	add_test(NAME ${test_name} COMMAND ${test_name})
endfunction(addtest)

addtest(interline_memory interline_memory.cpp)
addtest(strings_solid solid.cpp)

# create targets for all tests and for every manual one
#add_executable(run_test ${TEST_SRC})
#target_link_libraries(
#	run_test
#	daemon_lib cdl microhttpd sqlite3 systemd pthread z ssl crypto gtest ${GTEST_LIBRARIES} ${#GTEST_MAIN_LIBRARIES} ${GMOCK_BOTH_LIBRARIES} Threads::Threads
#)

