set(CTEST_SOURCE_DIRECTORY .)
set(CTEST_BINARY_DIRECTORY .)
set(CTEST_COMMAND ctest)
execute_process(COMMAND ctest -D NightlyStart)
execute_process(COMMAND ctest -D NightlyUpdate)
execute_process(COMMAND cmake . -DENABLE_COVERAGE:BOOL=ON)
execute_process(COMMAND ctest -D NightlyBuild -C Debug)
execute_process(COMMAND ctest -D NightlyTest -C Debug)
execute_process(COMMAND ctest -D NightlyMemCheck)
execute_process(COMMAND ctest -D NightlyCoverage)
execute_process(COMMAND ctest -D NightlySubmit)
