/*
 *  Off-the-Record Messaging plugin for pidgin
 *  Copyright (C) 2004-2018  Ian Goldberg, Rob Smits,
 *                           Chris Alexander, Willy Lew,
 *                           Nikita Borisov
 *                           <otr@cypherpunks.ca>
 *                           The pidgin-otrng contributors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "prekey-plugin.h"
#include "prekey-plugin-account.h"
#include "prekey-plugin-peers.h"
#include "prekey-plugin-shared.h"

/* If we're using glib on Windows, we need to use g_fopen to open files.
 * On other platforms, it's also safe to use it.  If we're not using
 * glib, just use fopen. */
#ifdef USING_GTK
/* If we're cross-compiling, this might be wrong, so fix it. */
#ifdef WIN32
#undef G_OS_UNIX
#define G_OS_WIN32
#endif
#include <glib/gstdio.h>
#else
#define g_fopen fopen
#endif

#ifdef ENABLE_NLS
/* internationalisation header */
#include <glib/gi18n-lib.h>
#else
#define _(x) (x)
#define N_(x) (x)
#endif

/* libpurple */
#include <connection.h>
#include <prpl.h>

#include <libotr-ng/alloc.h>
#include <libotr-ng/client_orchestration.h>
#include <libotr-ng/debug.h>
#include <libotr-ng/deserialize.h>
#include <libotr-ng/messaging.h>
#include <libotr-ng/prekey_manager.h>

#include "pidgin-helpers.h"
#include "prekey-discovery.h"

extern otrng_global_state_s *otrng_state;

static void notify_error_cb(otrng_client_s *client, int error, void *ctx) {
  otrng_debug_fprintf(stderr, "[%s] Prekey Server: an error happened: %d\n",
                      client->client_id.account, error);
}

static const char *domain_for_account_cb(otrng_client_s *client, void *ctx) {
  PurpleAccount *account = ctx;
  return otrng_plugin_prekey_domain_for(account,
                                        purple_account_get_username(account));
}

static void set_prekey_client_callbacks(otrng_client_s *client) {
  client->prekey_manager->callbacks->notify_error = notify_error_cb;
  client->prekey_manager->callbacks->storage_status_received =
      storage_status_received_cb;
  client->prekey_manager->callbacks->success_received = success_received_cb;
  client->prekey_manager->callbacks->failure_received = failure_received_cb;
  client->prekey_manager->callbacks->no_prekey_in_storage_received =
      no_prekey_in_storage_received_cb;
  client->prekey_manager->callbacks->low_prekey_messages_in_storage =
      low_prekey_messages_in_storage_cb;
  client->prekey_manager->callbacks->prekey_ensembles_received =
      prekey_ensembles_received_cb;
  client->prekey_manager->callbacks->build_prekey_publication_message =
      build_prekey_publication_message_cb;
  client->prekey_manager->callbacks->domain_for_account = domain_for_account_cb;
};

void otrng_prekey_plugin_ensure_prekey_manager(otrng_client_s *client) {
  if (otrng_prekey_ensure_manager(client, client->client_id.account) ==
      otrng_true) {
    set_prekey_client_callbacks(client);
  }
}

static gboolean receive_prekey_protocol_message(char **tosend,
                                                const char *server,
                                                const char *message,
                                                PurpleAccount *account) {
  otrng_client_s *client =
      otrng_client_get(otrng_state, purple_account_to_client_id(account));
  if (!client) {
    return FALSE;
  }

  return otrng_prekey_receive(tosend, client, server, message);
}

static gboolean receiving_im_msg_cb(PurpleAccount *account, char **who,
                                    char **message, PurpleConversation *conv,
                                    PurpleMessageFlags *flags) {

  if (!who || !*who || !message || !*message) {
    return 0;
  }

  char *username = g_strdup(purple_normalize(account, *who));

  char *tosend = NULL;
  gboolean ignore =
      receive_prekey_protocol_message(&tosend, username, *message, account);
  free(username);

  if (tosend) {
    send_message(account, *who, tosend);
    free(tosend);
  }

  // We consumed the message
  if (ignore) {
    free(*message);
    *message = NULL;
  }

  return ignore;
}

gboolean otrng_prekey_plugin_load(PurplePlugin *handle) {
  otrng_debug_enter("otrng_prekey_plugin_load");
  if (!otrng_state) {
    otrng_debug_exit("otrng_prekey_plugin_load");
    return FALSE;
  }

  /* Process received prekey protocol messages */
  purple_signal_connect(purple_conversations_get_handle(), "receiving-im-msg",
                        handle, PURPLE_CALLBACK(receiving_im_msg_cb), NULL);

  otrng_prekey_plugin_account_load(handle);
  otrng_prekey_plugin_peers_load(handle);

  // Do the same on the already connected accounts
  // GList *connections = purple_connections_get_all();
  otrng_debug_exit("otrng_prekey_plugin_load");
  return TRUE;
}

gboolean otrng_prekey_plugin_unload(PurplePlugin *handle) {
  otrng_debug_enter("otrng_prekey_plugin_unload");
  otrng_prekey_plugin_peers_unload(handle);
  otrng_prekey_plugin_account_unload(handle);

  purple_signal_disconnect(purple_conversations_get_handle(),
                           "receiving-im-msg", handle,
                           PURPLE_CALLBACK(receiving_im_msg_cb));

  otrng_debug_exit("otrng_prekey_plugin_unload");
  return TRUE;
}
