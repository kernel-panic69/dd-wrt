<?php declare(strict_types = 0);
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


/**
 * @var CView $this
 */
?>

<script>
	const view = {
		delete_url: null,

		init({delete_url}) {
			this.delete_url = delete_url;

			document.addEventListener('click', (e) => {
				if (e.target.classList.contains('js-create-templategroup')) {
					this.edit();
				}
				else if (e.target.classList.contains('js-edit-templategroup')) {
					e.preventDefault();
					this.edit({groupid: e.target.dataset.groupid});
				}
				else if (e.target.classList.contains('js-massdelete-templategroup')) {
					this.delete(e.target, Object.keys(chkbxRange.getSelectedIds()));
				}
			});
		},

		edit(parameters = {}) {
			const original_url = location.href;
			const overlay = PopUp('popup.templategroup.edit', parameters, {
				dialogueid: 'templategroup_edit',
				dialogue_class: 'modal-popup-static',
				prevent_navigation: true
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', (e) => this._reload(e.detail));
			overlay.$dialogue[0].addEventListener('dialogue.delete', (e) => this._reload(e.detail));
			overlay.$dialogue[0].addEventListener('overlay.close', () => {
				history.replaceState({}, '', original_url);
			}, {once: true});
		},

		delete(target, groupids) {
			const confirmation = groupids.length > 1
				? <?= json_encode(_('Delete selected template groups?')) ?>
				: <?= json_encode(_('Delete selected template group?')) ?>;

			if (!window.confirm(confirmation)) {
				return;
			}

			this._post(target, groupids, this.delete_url);
		},

		_post(target, groupids, url) {
			target.classList.add('is-loading');

			return fetch(url, {
				method: 'POST',
				headers: {'Content-Type': 'application/json'},
				body: JSON.stringify({groupids})
			})
				.then((response) => response.json())
				.then((response) => {
					if ('error' in response) {
						if ('title' in response.error) {
							postMessageError(response.error.title);
						}

						postMessageDetails('error', response.error.messages);

						uncheckTableRows('templategroup', response.error.keepids);
					}
					else if ('success' in response) {
						postMessageOk(response.success.title);

						if ('messages' in response.success) {
							postMessageDetails('success', response.success.messages);
						}

						uncheckTableRows('templategroup');
					}

					location.href = location.href;
				})
				.catch(() => {
					clearMessages();

					const title = <?= json_encode(_('Unexpected server error.')) ?>;
					const message_box = makeMessageBox('bad', [], title, true, false)[0];

					addMessage(message_box);
				})
				.finally(() => {
					target.classList.remove('is-loading');
				});
		},

		_reload(success) {
			postMessageOk(success.title);

			if ('messages' in success) {
				postMessageDetails('success', success.messages);
			}

			uncheckTableRows('templategroup');
			location.href = location.href;
		}
	}
</script>
