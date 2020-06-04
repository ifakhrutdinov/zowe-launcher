#!/bin/sh

################################################################################
#  This program and the accompanying materials are
#  made available under the terms of the Eclipse Public License v2.0 which accompanies
#  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html
#
#  SPDX-License-Identifier: EPL-2.0
#
#  Copyright Contributors to the Zowe Project.
################################################################################

echo "hello from shell test 2"

ls .

java foobar

while true
do
  echo "Doing shell script component test 2"
  sleep 10
done

echo "leaving shell test 2"

