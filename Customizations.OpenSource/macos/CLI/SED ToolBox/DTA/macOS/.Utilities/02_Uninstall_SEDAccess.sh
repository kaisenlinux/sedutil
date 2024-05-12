#!/bin/bash

# Source Utility_functions.sh from the same directory as this script
. "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/Utility_functions.sh"

beroot

launch_agent_dirs="/Library/LaunchAgents /System/Library/LaunchAgents ${HOME}/Library/LaunchAgents"
agent_app_dirs="/usr/libexec /usr/local/libexec"

old_app_names="sedagent"
new_app_names="SEDAccess"

for app_name in ${old_app_names} ${new_app_names}
do
    for dir in ${launch_agent_dirs}
    do
        plist=${dir}/com.brightplaza.${app_name}.plist
        if [ -f "${plist}" ]
        then
            DEBUG_PRINT "Unload ${plist}"
            2>/dev/null launchctl unload ${plist}
            DEBUG_PRINT "Remove ${plist}"
            rm -f ${plist}
        fi
    done

    PID="$(ps aux | grep ${app_name}.app | grep -v grep | awk '{print $2}')"
    if [ -n "${PID}" ]
    then
        kill -9 "${PID}"
    fi

    for dir in ${agent_app_dirs}
    do
        if [ -d "${dir}/${app_name}.app" ]
        then
            DEBUG_PRINT "Remove ${dir}/${app_name}.app"
            rm -rf ${dir}/${app_name}.app
        fi
    done
done


exit 0
