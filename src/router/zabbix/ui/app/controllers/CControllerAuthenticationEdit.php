<?php
/*
** Zabbix
** Copyright (C) 2001-2024 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/


class CControllerAuthenticationEdit extends CController {

	protected function init() {
		$this->disableCsrfValidation();
	}

	/**
	 * Validate user input.
	 *
	 * @return bool
	 */
	protected function checkInput() {
		$fields = [
			'form_refresh' =>					'int32',
			'authentication_type' =>			'in '.ZBX_AUTH_INTERNAL.','.ZBX_AUTH_LDAP,
			'disabled_usrgrpid' =>				'id',
			'http_auth_enabled' =>				'in '.ZBX_AUTH_HTTP_DISABLED.','.ZBX_AUTH_HTTP_ENABLED,
			'http_login_form' =>				'in '.ZBX_AUTH_FORM_ZABBIX.','.ZBX_AUTH_FORM_HTTP,
			'http_strip_domains' =>				'db config.http_strip_domains',
			'http_case_sensitive' =>			'in '.ZBX_AUTH_CASE_INSENSITIVE.','.ZBX_AUTH_CASE_SENSITIVE,
			'ldap_auth_enabled' =>				'in '.ZBX_AUTH_LDAP_DISABLED.','.ZBX_AUTH_LDAP_ENABLED,
			'ldap_servers' =>					'array',
			'ldap_default_row_index' =>			'int32',
			'ldap_case_sensitive' =>			'in '.ZBX_AUTH_CASE_INSENSITIVE.','.ZBX_AUTH_CASE_SENSITIVE,
			'ldap_removed_userdirectoryids' =>	'array_id',
			'jit_provision_interval' =>			'db config.jit_provision_interval',
			'ldap_jit_status' =>				'in '.JIT_PROVISIONING_DISABLED.','.JIT_PROVISIONING_ENABLED,
			'saml_auth_enabled' =>				'in '.ZBX_AUTH_SAML_DISABLED.','.ZBX_AUTH_SAML_ENABLED,
			'saml_jit_status' =>				'in '.JIT_PROVISIONING_DISABLED.','.JIT_PROVISIONING_ENABLED,
			'idp_entityid' =>					'db userdirectory_saml.idp_entityid',
			'sso_url' =>						'db userdirectory_saml.sso_url',
			'slo_url' =>						'db userdirectory_saml.slo_url',
			'username_attribute' =>				'db userdirectory_saml.username_attribute',
			'sp_entityid' =>					'db userdirectory_saml.sp_entityid',
			'nameid_format' =>					'db userdirectory_saml.nameid_format',
			'sign_messages' =>					'in 0,1',
			'sign_assertions' =>				'in 0,1',
			'sign_authn_requests' =>			'in 0,1',
			'sign_logout_requests' =>			'in 0,1',
			'sign_logout_responses' =>			'in 0,1',
			'encrypt_nameid' =>					'in 0,1',
			'encrypt_assertions' =>				'in 0,1',
			'saml_provision_status' =>			'in '.JIT_PROVISIONING_DISABLED.','.JIT_PROVISIONING_ENABLED,
			'saml_case_sensitive' =>			'in '.ZBX_AUTH_CASE_INSENSITIVE.','.ZBX_AUTH_CASE_SENSITIVE,
			'saml_group_name' =>				'db userdirectory_saml.group_name',
			'saml_user_username' =>				'db userdirectory_saml.user_username',
			'saml_user_lastname' =>				'db userdirectory_saml.user_lastname',
			'saml_provision_groups' =>			'array',
			'saml_provision_media' =>			'array',
			'scim_status' =>					'in '.ZBX_AUTH_SCIM_PROVISIONING_DISABLED.','.ZBX_AUTH_SCIM_PROVISIONING_ENABLED,
			'passwd_min_length' =>				'int32',
			'passwd_check_rules' =>				'int32|ge 0|le '.(PASSWD_CHECK_CASE | PASSWD_CHECK_DIGITS | PASSWD_CHECK_SPECIAL | PASSWD_CHECK_SIMPLE)
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(new CControllerResponseFatal());
		}

		return $ret;
	}

	/**
	 * Validate is user allowed to change configuration.
	 *
	 * @return bool
	 */
	protected function checkPermissions() {
		return $this->checkAccess(CRoleHelper::UI_ADMINISTRATION_AUTHENTICATION);
	}

	protected function doAction() {
		$ldap_status = (new CFrontendSetup())->checkPhpLdapModule();
		$openssl_status = (new CFrontendSetup())->checkPhpOpenSsl();

		$data = [
			'action_submit' => 'authentication.update',
			'action_passw_change' => 'authentication.edit',
			'ldap_error' => ($ldap_status['result'] == CFrontendSetup::CHECK_OK) ? '' : $ldap_status['error'],
			'saml_error' => ($openssl_status['result'] == CFrontendSetup::CHECK_OK) ? '' : $openssl_status['error'],
			'form_refresh' => $this->getInput('form_refresh', 0)
		];

		$auth_params = [
			CAuthenticationHelper::AUTHENTICATION_TYPE,
			CAuthenticationHelper::DISABLED_USER_GROUPID,
			CAuthenticationHelper::HTTP_AUTH_ENABLED,
			CAuthenticationHelper::HTTP_LOGIN_FORM,
			CAuthenticationHelper::HTTP_STRIP_DOMAINS,
			CAuthenticationHelper::HTTP_CASE_SENSITIVE,
			CAuthenticationHelper::LDAP_AUTH_ENABLED,
			CAuthenticationHelper::LDAP_USERDIRECTORYID,
			CAuthenticationHelper::LDAP_CASE_SENSITIVE,
			CAuthenticationHelper::LDAP_JIT_STATUS,
			CAuthenticationHelper::JIT_PROVISION_INTERVAL,
			CAuthenticationHelper::SAML_AUTH_ENABLED,
			CAuthenticationHelper::SAML_JIT_STATUS,
			CAuthenticationHelper::SAML_CASE_SENSITIVE,
			CAuthenticationHelper::PASSWD_MIN_LENGTH,
			CAuthenticationHelper::PASSWD_CHECK_RULES
		];
		$auth = [];
		foreach ($auth_params as $param) {
			$auth[$param] = CAuthenticationHelper::get($param);
		}

		if ($this->hasInput('form_refresh')) {
			$config_fields = [
				'authentication_type' => DB::getDefault('config', 'authentication_type'),
				'disabled_usrgrpid' => 0,
				'http_auth_enabled' => DB::getDefault('config', 'http_auth_enabled'),
				'http_login_form' => DB::getDefault('config', 'http_login_form'),
				'http_strip_domains' => DB::getDefault('config', 'http_strip_domains'),
				'http_case_sensitive' => ZBX_AUTH_CASE_INSENSITIVE,
				'ldap_auth_enabled' => ZBX_AUTH_LDAP_DISABLED,
				'ldap_case_sensitive' => ZBX_AUTH_CASE_INSENSITIVE,
				'jit_provision_interval' => $auth[CAuthenticationHelper::JIT_PROVISION_INTERVAL],
				'ldap_jit_status' => ZBX_AUTH_LDAP_DISABLED,
				'saml_auth_enabled' => ZBX_AUTH_SAML_DISABLED,
				'saml_jit_status' => JIT_PROVISIONING_DISABLED,
				'idp_entityid' => '',
				'sso_url' => '',
				'slo_url' => '',
				'username_attribute' => '',
				'sp_entityid' => '',
				'nameid_format' => '',
				'sign_messages' => 0,
				'sign_assertions' => 0,
				'sign_authn_requests' => 0,
				'sign_logout_requests' => 0,
				'sign_logout_responses' => 0,
				'encrypt_nameid' => 0,
				'encrypt_assertions' => 0,
				'saml_case_sensitive' => ZBX_AUTH_CASE_INSENSITIVE,
				'saml_group_name' => '',
				'saml_user_username' => '',
				'saml_user_lastname' => '',
				'scim_status' => ZBX_AUTH_SCIM_PROVISIONING_DISABLED,
				'passwd_min_length' => '',
				'passwd_check_rules' => 0
			];
			$this->getInputs($data, array_keys($config_fields));
			$data += $config_fields;

			$data['saml_provision_status'] = $this->getInput('saml_provision_status', JIT_PROVISIONING_DISABLED);
			$data['saml_provision_groups'] = $this->getInput('saml_provision_groups', []);
			$data['saml_provision_media'] = $this->getInput('saml_provision_media', []);

			self::extendProvisionGroups($data['saml_provision_groups']);
			self::extendProvisionMedia($data['saml_provision_media']);

			$data['ldap_servers'] = $this->getLdapServerUserGroupCount($this->getInput('ldap_servers', []));
			$data['ldap_default_row_index'] = $this->getInput('ldap_default_row_index', 0);
			$data['ldap_removed_userdirectoryids'] = $this->getInput('ldap_removed_userdirectoryids', []);

			$data += $auth;
		}
		else {
			$data += $auth;

			$userdirectories = API::UserDirectory()->get([
				'output' => API_OUTPUT_EXTEND,
				'selectProvisionGroups' => API_OUTPUT_EXTEND,
				'selectProvisionMedia' => API_OUTPUT_EXTEND,
				'selectUsrgrps' => API_OUTPUT_COUNT,
				'sortfield' => ['name']
			]);

			$saml_configuration = [];
			$data['ldap_servers'] = [];
			foreach ($userdirectories as $userdirectory) {
				if ($userdirectory['idp_type'] == IDP_TYPE_SAML) {
					$saml_configuration = $userdirectory;
				}
				else {
					$data['ldap_servers'][] = $userdirectory;
				}
			}

			if ($saml_configuration) {
				$saml_configuration = CArrayHelper::renameKeys($saml_configuration, [
					'provision_status' => 'saml_provision_status',
					'group_name' => 'saml_group_name',
					'user_username' => 'saml_user_username',
					'user_lastname' => 'saml_user_lastname',
					'provision_groups' => 'saml_provision_groups',
					'provision_media' => 'saml_provision_media'
				]);
			}
			else {
				$saml_configuration = [
					'idp_entityid' => '',
					'sso_url' => '',
					'slo_url' => '',
					'username_attribute' => '',
					'sp_entityid' => '',
					'nameid_format' => '',
					'sign_messages' => '',
					'sign_assertions' => '',
					'sign_authn_requests' => '',
					'sign_logout_requests' => '',
					'sign_logout_responses' => '',
					'encrypt_nameid' => '',
					'encrypt_assertions' => '',
					'saml_provision_status' => JIT_PROVISIONING_DISABLED,
					'saml_group_name' => '',
					'saml_user_username' => '',
					'saml_user_lastname' => '',
					'scim_status' => '',
					'saml_provision_groups' => [],
					'saml_provision_media' => []
				];
			}

			self::extendProvisionGroups($saml_configuration['saml_provision_groups']);
			self::extendProvisionMedia($saml_configuration['saml_provision_media']);

			$data += $saml_configuration;

			// Cast false to 0 when no ldap server is found as default.
			$data['ldap_default_row_index'] = (int) array_search($data[CAuthenticationHelper::LDAP_USERDIRECTORYID],
				array_column($data['ldap_servers'], 'userdirectoryid')
			);
			$data['ldap_removed_userdirectoryids'] = [];
		}

		unset($data[CAuthenticationHelper::LDAP_USERDIRECTORYID]);
		$data['ldap_enabled'] = ($ldap_status['result'] == CFrontendSetup::CHECK_OK
				&& $data['ldap_auth_enabled'] == ZBX_AUTH_LDAP_ENABLED
		);
		$data['saml_enabled'] = ($openssl_status['result'] == CFrontendSetup::CHECK_OK
				&& $data['saml_auth_enabled'] == ZBX_AUTH_SAML_ENABLED
		);
		$data['db_authentication_type'] = CAuthenticationHelper::get(CAuthenticationHelper::AUTHENTICATION_TYPE);
		$data['disabled_usrgrpid_ms'] = [];

		if ($data['disabled_usrgrpid']) {
			$groups = API::UserGroup()->get([
				'output' => ['usrgrpid', 'name'],
				'usrgrpids' => [$data['disabled_usrgrpid']]
			]);
			$data['disabled_usrgrpid_ms'] = CArrayHelper::renameObjectsKeys($groups, ['usrgrpid' => 'id']);
		}

		$response = new CControllerResponseData($data);
		$response->setTitle(_('Configuration of authentication'));
		$this->setResponse($response);
	}

	private function getLdapServerUserGroupCount(array $ldap_servers): array {
		$ldap_serverids = array_column($ldap_servers, 'userdirectoryid');

		$db_ldap_servers = $ldap_serverids
			? API::UserDirectory()->get([
				'output' => [],
				'selectUsrgrps' => API_OUTPUT_COUNT,
				'userdirectoryids' => $ldap_serverids,
				'filter' => [
					'idp_type' => IDP_TYPE_LDAP
				],
				'preservekeys' => true
			])
			: [];

		foreach ($ldap_servers as &$ldap_server) {
			$ldap_server['usrgrps'] = 0;

			if (array_key_exists('userdirectoryid', $ldap_server)
					&& array_key_exists($ldap_server['userdirectoryid'], $db_ldap_servers)) {
				$ldap_server['usrgrps'] = $db_ldap_servers[$ldap_server['userdirectoryid']]['usrgrps'];
			}
		}
		unset($ldap_server);

		return $ldap_servers;
	}

	/**
	 * Adds missing information that is necessary for provision group rendering in the view.
	 *
	 * @param array $provision_groups
	 */
	private static function extendProvisionGroups(array &$provision_groups): void {
		if (!$provision_groups) {
			return;
		}

		$roleids = [];
		$usrgrpids = [];

		foreach ($provision_groups as $group) {
			if (array_key_exists('roleid', $group)) {
				$roleids[$group['roleid']] = $group['roleid'];
			}

			if (array_key_exists('user_groups', $group)) {
				foreach ($group['user_groups'] as $user_group) {
					$usrgrpids[$user_group['usrgrpid']] = $user_group['usrgrpid'];
				}
			}
		}

		$roles = $roleids
			? API::Role()->get([
				'output' => ['name'],
				'roleids' => $roleids,
				'preservekeys' => true
			])
			: [];

		$user_groups = $usrgrpids
			? API::UserGroup()->get([
				'output' => ['name'],
				'usrgrpids' => $usrgrpids,
				'preservekeys' => true
			])
			: [];

		foreach ($provision_groups as $index => &$provision_group) {
			if (array_key_exists('roleid', $provision_group) && array_key_exists($provision_group['roleid'], $roles)) {
				$provision_group['role_name'] = $roles[$provision_group['roleid']]['name'];
			}
			else {
				unset($provision_groups[$index]);
			}

			if (array_key_exists('user_groups', $provision_group)) {
				foreach ($provision_group['user_groups'] as $i => &$user_group) {
					if (array_key_exists($user_group['usrgrpid'], $user_groups)) {
						$user_group['name'] = $user_groups[$user_group['usrgrpid']]['name'];
					}
					else {
						unset($provision_group['user_groups'][$i]);
					}
				}
				unset($user_group);
			}
		}
		unset($provision_group);
	}

	/**
	 * Adds missing information that is necessary for provision media rendering in the view.
	 *
	 * @param array $provision_media
	 */
	private static function extendProvisionMedia(array &$provision_media): void {
		if (!$provision_media) {
			return;
		}

		$db_media = API::MediaType()->get([
			'output' => ['name'],
			'mediatypeids' => array_column($provision_media, 'mediatypeid'),
			'preservekeys' => true
		]);

		foreach ($provision_media as $index => $media) {
			if (!array_key_exists($media['mediatypeid'], $db_media)) {
				unset($provision_media[$index]);
				continue;
			}

			$provision_media[$index]['mediatype_name'] = $db_media[$media['mediatypeid']]['name'];
		}
	}
}
