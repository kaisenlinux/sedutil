#!/bin/bash
# set -xv

#echo "Original source for this script was /Users/scott/Drive Trust Alliance/DTA/submodules/FH/sedutil/macos/SED ToolBox/DTA/macOS/.Utilities/51_Uninstall_security_items.sh"

#echo "this script dir=${dir}"

. "${dir}/Utility_functions.sh"

beroot

export CA_NAME="Bright Plaza CA"

export KEYCHAIN_NAME="Bright Plaza SED"
export KEYCHAIN_PASSWORD_LABEL="${KEYCHAIN_NAME} Keychain"
export KEYCHAIN_PASSWORD_SERVICE="com.BrightPlaza.SED"
export SERVER_NAME="Bright Plaza SED Server"

export SYSTEM_KEYCHAIN_DIR="/Library/Keychains"
export SYSTEM_KEYCHAIN_PATH="${SYSTEM_KEYCHAIN_DIR}/System.keychain"
export KEYCHAIN_PATH="${SYSTEM_KEYCHAIN_DIR}/${KEYCHAIN_NAME}.keychain"


function file_exists {
    [ -f "${1}" ]
}
export -f file_exists
        
function SED_keychain_exists {
    file_exists "${KEYCHAIN_PATH}"
}


function perform_trusted_security_operation {
    security authorizationdb read com.apple.trust-settings.admin > /tmp/security.plist
    security authorizationdb write com.apple.trust-settings.admin admin
    local -i result
    security "$@"
    result=$?
    security authorizationdb write com.apple.trust-settings.admin < /tmp/security.plist
    rm /tmp/security.plist 
    return ${result}
}


function find_securityService_password {
    local -i result
    security -v find-generic-password -l "com.brightplaza.securityService" -s "com.brightplaza.securityService" 
    result=$?
    DEBUG_PRINT "find-generic-password return code is $result"
    return ${result}
}
export -f find_securityService_password 

function delete_securityService_password {
    local -i result
    security -v delete-generic-password -l "com.brightplaza.securityService" -s "com.brightplaza.securityService" 
    result=$?
    DEBUG_PRINT "delete-generic-password return code is $result"
    return ${result}
}
export -f delete_securityService_password 

function remove_securityService_password {
    find_securityService_password >> "${DEBUGGING_OUTPUT}" 2>&1 \
        || return $( DEBUG_PRINT "No securityService password" )
    delete_securityService_password  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        || return $( DEBUG_FAILURE_RETURN "deleting securityService password" )
}
export -f remove_securityService_password 


function read_SED_keychain_password_from_system_keychain {
   2>/dev/null sudo security find-generic-password -l "${KEYCHAIN_PASSWORD_LABEL}" -w "${SYSTEM_KEYCHAIN_PATH}"
}
export -f read_SED_keychain_password_from_system_keychain
    
function unlock_SED_keychain {
    2>/dev/null sudo security unlock-keychain -p "${KEYCHAIN_PASSWORD}"  "${KEYCHAIN_PATH}"
}
export -f unlock_SED_keychain
    
function delete_SED_keychain {
    2>/dev/null sudo security delete-keychain  "${KEYCHAIN_PATH}"
}
export -f delete_SED_keychain
    
function remove_SED_keychain {
    SED_keychain_exists  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        || return $( DEBUG_PRINT "No SED keychain present." )
    KEYCHAIN_PASSWORD="$( read_SED_keychain_password_from_system_keychain )" \
        || return $( DEBUG_FAILURE_RETURN "finding System keychain item \"${KEYCHAIN_PASSWORD_LABEL}\"" )
    unlock_SED_keychain  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        || return $( DEBUG_FAILURE_RETURN "unlocking \"${KEYCHAIN_PATH}\"" )
    delete_SED_keychain  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        || return $( DEBUG_FAILURE_RETURN "deleting \"${KEYCHAIN_PATH}\"" )
}
export -f remove_SED_keychain

function system_keychain_has_SED_keychain_password {
    local -i result
    export KEYCHAIN_PASSWORD
    KEYCHAIN_PASSWORD="$( read_SED_keychain_password_from_system_keychain )"
    result=$?
    (( $result==0 )) && [ -n "${KEYCHAIN_PASSWORD}" ] || result=1
    return ${result}
}
export -f system_keychain_has_SED_keychain_password

function delete_SED_keychain_password_from_system_keychain {
    security delete-generic-password -l "${KEYCHAIN_PASSWORD_LABEL}"  "${SYSTEM_KEYCHAIN_PATH}"
}
export -f delete_SED_keychain_password_from_system_keychain

function remove_SED_keychain_password {
    DEBUG_PRINT "remove_SED_keychain_password"
    system_keychain_has_SED_keychain_password  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        ||  return $( DEBUG_PRINT "No SED keychain password present." )
    delete_SED_keychain_password_from_system_keychain  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        ||  return $( DEBUG_FAILURE_RETURN  "deleting System keychain item \"${KEYCHAIN_PASSWORD_LABEL}\"" )
}
export -f remove_SED_keychain_password


function system_keychain_has_CA_cert {
    DEBUG_PRINT "system_keychain_has_CA_cert"
#    1>/dev/null 2>&1 security find-certificate -c "${CA_NAME}"  "${SYSTEM_KEYCHAIN_PATH}"
     security find-certificate -c "${CA_NAME}"  "${SYSTEM_KEYCHAIN_PATH}"
}
export -f system_keychain_has_CA_cert

function delete_CA_cert_from_system_keychain {
    CAFILENAME="${CA_NAME/\*\./}"
    CERTIFICATES_DIR_PATH="$(realpath "${dir}/../Certificates")"
    CAFILEPATH="${CERTIFICATES_DIR_PATH}/${CAFILENAME}.pem"
    DEBUG_PRINT "CAFILENAME=${CAFILENAME} ---> CERTIFICATES_DIR_PATH=${CERTIFICATES_DIR_PATH}"
    DEBUG_PRINT "---> CAFILEPATH=${CAFILEPATH}"
    local -i result
    perform_trusted_security_operation remove-trusted-cert -d "${CAFILEPATH}"
    result=$?
    DEBUG_PRINT "remove-trusted-cert result=${result}"
    perform_trusted_security_operation delete-certificate -c "${CA_NAME}"
    result=$?
    DEBUG_PRINT "delete-certificate result=${result}"
    return ${result}
}
export -f delete_CA_cert_from_system_keychain

function remove_CA_cert_from_system_keychain {
    DEBUG_PRINT "remove_CA_cert_from_system_keychain "
    system_keychain_has_CA_cert  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        ||  return $( DEBUG_PRINT "No CA cert present." )
    delete_CA_cert_from_system_keychain  >> "${DEBUGGING_OUTPUT}" 2>&1 \
        ||  return $( DEBUG_FAILURE_RETURN  "deleting CA cert from System keychain" )
}
export -f remove_CA_cert_from_system_keychain




#set -xv
DEBUG_PRINT "Removing security items"
remove_securityService_password  >> "${DEBUGGING_OUTPUT}" 2>&1       \
        || DEBUG_FAIL "removing securityService password" 10
remove_SED_keychain  >> "${DEBUGGING_OUTPUT}" 2>&1       \
        || DEBUG_FAIL "removing SED keychain" 10
remove_SED_keychain_password  >> "${DEBUGGING_OUTPUT}" 2>&1       \
        || DEBUG_FAIL "removing SED keychain password from system keychain" 10
remove_CA_cert_from_system_keychain  >> "${DEBUGGING_OUTPUT}" 2>&1       \
        || DEBUG_FAIL "removing CA cert from system keychain" 10
#set +xv
