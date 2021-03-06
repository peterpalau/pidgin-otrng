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

#ifndef __OTRG_PLUGIN_ALL_H__
#define __OTRG_PLUGIN_ALL_H__

/* Purple headers */
#include <account.h>
#include <plugin.h>

/* libotr headers */
#include <libotr/context.h>
#include <libotr/instag.h>
#include <libotr/userstate.h>

#include <libotr-ng/messaging.h>

/* libotrng headers */
#include "otrng-client.h"

#include "plugin-conversation.h"

#include "pidgin-helpers.h"

#define PRIVKEY_FILE_NAME "otr.private_key"
#define INSTAG_FILE_NAME "otr.instance_tags"
#define MAX_MSG_SIZE_FILE_NAME "otr.max_message_size"

extern PurplePlugin *otrng_plugin_handle;

extern otrng_global_state_s *otrng_state;

/* Given a PurpleConversation, return the ConnContext corresponding to the
 * selected instance tag. */
ConnContext *otrng_plugin_conv_to_selected_context(PurpleConversation *conv,
                                                   int force_create);

/* Given a PurpleConversation, return the selected instag. */
otrl_instag_t otrng_plugin_conv_to_selected_instag(PurpleConversation *conv,
                                                   otrl_instag_t default_val);

/* Send an IM from the given account to the given recipient.  Display an
 * error dialog if that account isn't currently logged in. */
void otrng_plugin_inject_message(PurpleAccount *account, const char *recipient,
                                 const char *message);

/* Generate a instance tag for the given accountname/protocol */
void otrng_plugin_create_instag(const PurpleAccount *account);

// TODO: REPLACE by using opdata to get this information

otrng_plugin_conversation *
purple_conversation_to_plugin_conversation(const PurpleConversation *conv);

int otrng_plugin_buddy_is_offline(PurpleAccount *account, PurpleBuddy *buddy);

void otrng_plugin_send_non_interactive_auth(const char *username,
                                            PurpleAccount *account);

otrng_client_s *
otrng_plugin_conversation_to_client(const otrng_plugin_conversation *conv);

otrng_plugin_conversation *
otrng_plugin_conversation_copy(const otrng_plugin_conversation *);

/* Start the Socialist Millionaires' Protocol over the current connection,
 * using the given initial secret, and optionally a question to pass to
 * the buddy. */
void otrng_plugin_start_smp(otrng_plugin_conversation *conv,
                            const unsigned char *question, const size_t q_len,
                            const unsigned char *secret, size_t secretlen);

void otrng_plugin_continue_smp(otrng_plugin_conversation *conv,
                               const unsigned char *secret, size_t secretlen);

/* Abort the SMP protocol.  Used when malformed or unexpected messages
 * are received. */
void otrng_plugin_abort_smp(const otrng_plugin_conversation *conv);

void otrng_plugin_send_default_query(otrng_plugin_conversation *conv);

/* Send the default OTR Query message to the correspondent of the given
 * conversation. */
void otrng_plugin_send_default_query_conv(PurpleConversation *conv);

/* Disconnect a context, sending a notice to the other side, if
 * appropriate. */
void otrng_plugin_disconnect(otrng_plugin_conversation *conv);

/* Find the ConnContext appropriate to a given PurpleConversation. */
ConnContext *otrng_plugin_conv_to_context(PurpleConversation *conv,
                                          otrl_instag_t their_instance,
                                          int force_create);

/* Find the PurpleConversation appropriate to the given ConnContext.  If
 * one doesn't yet exist, create it if force_create is true. */
PurpleConversation *otrng_plugin_context_to_conv(ConnContext *context,
                                                 int force_create);

typedef enum {
  TRUST_NOT_PRIVATE,
  TRUST_UNVERIFIED,
  TRUST_PRIVATE,
  TRUST_FINISHED
} TrustLevel;

TrustLevel
otrng_plugin_conversation_to_trust(const otrng_plugin_conversation *conv);

/* What level of trust do we have in the privacy of this ConnContext? */
TrustLevel otrng_plugin_context_to_trust(ConnContext *context);

/* Return 1 if the given protocol supports OTR, 0 otherwise. */
int otrng_plugin_proto_supports_otr(const char *proto);

int otrng_plugin_conversation_to_protocol_version(
    const otrng_plugin_conversation *conv);

static inline PurpleConversation *
otrng_plugin_conversation_to_purple_conv(const otrng_plugin_conversation *conv,
                                         int force) {
  return otrng_plugin_userinfo_to_conv(conv->account, conv->protocol,
                                       conv->peer, force);
}

gboolean otrng_plugin_unload(PurplePlugin *handle);
gboolean otrng_plugin_load(PurplePlugin *handle);

#endif
