#!/bin/bash

source .travis.compiler.sh \
&& ccache --version \
&& ${CC} --version \
&& ${CXX} --version \
&& env \
&& cmake . \
           -DCMAKE_BUILD_TYPE=${TYPE} \
           ${CMAKE_OPT} \
           -DWITH_SSL=system -DWITH_ZLIB=system -DPLUGIN_AWS_KEY_MANAGEMENT=DYNAMIC -DAWS_SDK_EXTERNAL_PROJECT=ON \
&& make -j 6 \
&& cd mysql-test \
&& ./mtr --force --max-test-fail=20 --parallel=6 --testcase-timeout=2 \
         --suite=${MYSQL_TEST_SUITES} \
         --skip-test-list=unstable-tests \
         --skip-test=binlog.binlog_unsafe \
&& ccache --show-stats
