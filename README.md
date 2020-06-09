This program and the accompanying materials are
made available under the terms of the Eclipse Public License v2.0 which accompanies
this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

SPDX-License-Identifier: EPL-2.0

Copyright Contributors to the Zowe Project.

# Zowe Launcher

The following changes are needed for the POC to work (based on Zowe 1.11.0):

* {INSTANCE_DIR}/bin/zowe-start.sh
```diff
--- {OLD_INSTANCE_DIR}/bin/zowe-start.sh    2020-06-09 13:15:53 -0400
+++ {NEW_INSTANCE_DIR}/bin/zowe-start.sh    2020-06-08 03:28:01 -0400
@@ -3,5 +3,4 @@
 export INSTANCE_DIR=$(cd $(dirname $0)/../;pwd)
 . ${INSTANCE_DIR}/bin/internal/read-instance.sh

-${ROOT_DIR}/scripts/internal/opercmd "S ZWESVSTC,INSTANCE='"${INSTANCE_DIR}"',JOBNAME=${ZOWE_PREFIX}${ZOWE_INSTANCE}SV"
-echo Start command issued, check SDSF job log ...
+/bin/sh ${INSTANCE_DIR}/bin/internal/run-zowe.sh
```

* {INSTALL_DIR}/bin/internal/run-zowe.sh
```diff
--- {OLD_INSTALL_DIR}/bin/internal/run-zowe.sh     2020-06-09 13:15:08 -0400
+++ {NEW_INSTALL_DIR}/bin/internal/run-zowe.sh     2020-06-08 05:16:21 -0400
@@ -74,7 +74,7 @@
   KEYSTORE_TYPE="PKCS12"
 fi

-LAUNCH_COMPONENTS=""
+LAUNCH_COMPONENTS=$ZOWE_INIT_COMPONENTS
 export ZOWE_PREFIX=${ZOWE_PREFIX}${ZOWE_INSTANCE}
 ZOWE_DESKTOP=${ZOWE_PREFIX}DT

@@ -101,7 +101,6 @@
 if [[ $LAUNCH_COMPONENT_GROUPS == *"DESKTOP"* ]]
 then
   LAUNCH_COMPONENTS=app-server,${LAUNCH_COMPONENTS} #Make app-server the first component, so any extender plugins can use its config
-  PLUGINS_DIR=${WORKSPACE_DIR}/app-server/plugins
 fi
 #ZSS could be included separate to app-server, and vice-versa
 #But for simplicity of this script we have app-server prereq zss in DESKTOP
@@ -114,6 +113,11 @@
   mkdir -p ${STATIC_DEF_CONFIG_DIR}
 fi

+if [[ $LAUNCH_COMPONENTS == *"app-server"* ]]
+then
+  PLUGINS_DIR=${WORKSPACE_DIR}/app-server/plugins
+fi
+
 # Prepend directory path to all internal components
 INTERNAL_COMPONENTS=""
 for i in $(echo $LAUNCH_COMPONENTS | sed "s/,/ /g")
@@ -184,5 +188,5 @@

 for LAUNCH_COMPONENT in $(echo $LAUNCH_COMPONENTS | sed "s/,/ /g")
 do
-  . ${LAUNCH_COMPONENT}/start.sh & #app-server/start.sh doesn't run in background, so blocks other components from starting
+  . ${LAUNCH_COMPONENT}/start.sh
 done
```

* {INSTALL_DIR}/components/api-mediation/bin/start.sh
```diff
--- {OLD_INSTALL_DIR}/components/api-mediation/bin/start.sh  2020-05-01 15:37:26 -0400
+++ {NEW_INSTALL_DIR}/components/api-mediation/bin/start.sh  2020-06-08 10:19:01 -0400
@@ -36,6 +36,15 @@
   APIML_STATIC_DEF="${APIML_STATIC_DEF};${ZWEAD_EXTERNAL_STATIC_DEF_DIRECTORIES}"
 fi

+stop_jobs()
+{
+  kill -15 $disc_pid $cat_pid $gateway_pid
+}
+
+trap 'stop_jobs' INT
+
 LIBPATH="$LIBPATH":"/lib"
 LIBPATH="$LIBPATH":"/usr/lib"
 LIBPATH="$LIBPATH":"${JAVA_HOME}"/bin
@@ -75,6 +82,7 @@
     -Dserver.ssl.trustStorePassword=${KEYSTORE_PASSWORD} \
     -Djava.protocol.handler.pkgs=com.ibm.crypto.provider \
     -jar ${ROOT_DIR}"/components/api-mediation/discovery-service.jar" &
+disc_pid=$?

 CATALOG_CODE=AC
 _BPX_JOBNAME=${ZOWE_PREFIX}${CATALOG_CODE} java -Xms16m -Xmx512m -Xquickstart \
@@ -103,6 +111,7 @@
     -Dserver.ssl.trustStorePassword=${KEYSTORE_PASSWORD} \
     -Djava.protocol.handler.pkgs=com.ibm.crypto.provider \
     -jar ${ROOT_DIR}"/components/api-mediation/api-catalog-services.jar" &
+cat_pid=$?

 GATEWAY_CODE=AG
 _BPX_JOBNAME=${ZOWE_PREFIX}${GATEWAY_CODE} java -Xms32m -Xmx256m -Xquickstart \
@@ -134,3 +143,7 @@
     -Djava.protocol.handler.pkgs=com.ibm.crypto.provider \
     -cp ${ROOT_DIR}"/components/api-mediation/gateway-service.jar":/usr/include/java_classes/IRRRacf.jar \
     org.springframework.boot.loader.PropertiesLauncher &
+gateway_pid=$?
+
+wait
+
```

* {INSTALL_DIR}/components/explorer-jes/bin/start.sh
```diff
--- {OLD_INSTALL_DIR}/components/explorer-jes/bin/start.sh   2020-04-27 17:44:09 -0400
+++ {NEW_INSTALL_DIR}/components/explorer-jes/bin/start.sh   2020-06-08 10:36:19 -0400
@@ -20,6 +20,13 @@
 # - KEYSTORE_CERTIFICATE
 # - ZOWE_PREFIX

+stop_jobs()
+{
+  kill -15 $pid
+}
+
+trap 'stop_jobs' INT
+
 NODE_BIN=${NODE_HOME}/bin/node

 cd "$ROOT_DIR/components/explorer-jes/bin"
@@ -40,3 +47,6 @@
        --cert ${KEYSTORE_CERTIFICATE} \
        --csp "${ZOWE_EXPLORER_HOST}:*" \
        -v &
+pid=$?
+
+wait
```

* {INSTALL_DIR}/components/explorer-mvs/bin/start.sh
```diff
--- {OLD_INSTALL_DIR}/components/explorer-mvs/bin/start.sh   2020-04-27 17:17:33 -0400
+++ {NEW_INSTALL_DIR}/components/explorer-mvs/bin/start.sh   2020-06-08 10:36:40 -0400
@@ -20,6 +20,13 @@
 # - KEYSTORE_CERTIFICATE
 # - ZOWE_PREFIX

+stop_jobs()
+{
+  kill -15 $pid
+}
+
+trap 'stop_jobs' INT
+
 NODE_BIN=${NODE_HOME}/bin/node

 cd "$ROOT_DIR/components/explorer-mvs/bin"
@@ -40,3 +47,6 @@
        --cert ${KEYSTORE_CERTIFICATE} \
        --csp "${ZOWE_EXPLORER_HOST}:*" \
        -v &
+pid=$?
+
+wait
```

* {INSTALL_DIR}/components/explorer-uss/bin/start.sh
```diff
--- {INSTALL_DIR}/components/explorer-uss/bin/start.sh   2020-04-27 18:36:50 -0400
+++ {INSTALL_DIR}/components/explorer-uss/bin/start.sh   2020-06-08 10:37:15 -0400
@@ -20,6 +20,13 @@
 # - KEYSTORE_CERTIFICATE
 # - ZOWE_PREFIX

+stop_jobs()
+{
+  kill -15 $pid
+}
+
+trap 'stop_jobs' INT
+
 NODE_BIN=${NODE_HOME}/bin/node

 cd "$ROOT_DIR/components/explorer-uss/bin"
@@ -40,3 +47,6 @@
        --cert $KEYSTORE_CERTIFICATE \
        --csp "${ZOWE_EXPLORER_HOST}:*" \
        -v &
+pid=$?
+
+wait
```

* {INSTALL_DIR}/components/files-api/bin/start.sh
```diff
--- {INSTALL_DIR}/components/files-api/bin/start.sh      2020-06-09 13:15:07 -0400
+++ {INSTALL_DIR}/components/files-api/bin/start.sh      2020-06-09 12:11:09 -0400
@@ -21,6 +21,15 @@
 # - GATEWAY_PORT - The SSL port z/OSMF is listening on.
 # - ZOWE_EXPLORER_HOST - The IP Address z/OSMF can be reached

+stop_jobs()
+{
+  kill -15 $pid
+}
+
+trap 'stop_jobs' INT
+
 COMPONENT_CODE=EF
 _BPX_JOBNAME=${ZOWE_PREFIX}${COMPONENT_CODE} java -Xms16m -Xmx512m -Dibm.serversocket.recover=true -Dfile.encoding=UTF-8 \
     -Djava.io.tmpdir=/tmp -Xquickstart \
@@ -36,3 +43,6 @@
     -Dconnection.ipAddress=${ZOWE_EXPLORER_HOST} \
     -Dspring.main.banner-mode=off \
     -jar ${ROOT_DIR}/components/files-api/bin/data-sets-api-server-1.0.0-boot.jar &
+pid=$?
+
+wait
```

* {INSTALL_DIR}/components/jobs-api/bin/start.sh
```diff
--- {INSTALL_DIR}/components/jobs-api/bin/start.sh       2020-06-09 13:15:07 -0400
+++ {INSTALL_DIR}/components/jobs-api/bin/start.sh       2020-06-09 12:11:28 -0400
@@ -21,6 +21,15 @@
 # - GATEWAY_PORT - The SSL port z/OSMF is listening on.
 # - ZOWE_EXPLORER_HOST - The IP Address z/OSMF can be reached

+stop_jobs()
+{
+  kill -15 $pid
+}
+
+trap 'stop_jobs' INT
+
 COMPONENT_CODE=EJ
 _BPX_JOBNAME=${ZOWE_PREFIX}${COMPONENT_CODE} java -Xms16m -Xmx512m -Dibm.serversocket.recover=true -Dfile.encoding=UTF-8 \
     -Djava.io.tmpdir=/tmp -Xquickstart \
@@ -37,3 +44,6 @@
     -Dconnection.ipAddress=${ZOWE_EXPLORER_HOST} \
     -Dspring.main.banner-mode=off \
     -jar ${ROOT_DIR}/components/jobs-api/bin/jobs-api-server-1.0.0-boot.jar &
+pid=$?
+
+wait
```

This program and the accompanying materials are
made available under the terms of the Eclipse Public License v2.0 which accompanies
this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

SPDX-License-Identifier: EPL-2.0

Copyright Contributors to the Zowe Project.

