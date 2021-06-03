//////////////////////////////////////////////////////////////////////////////
// Chats                                                                    //
// Have monsters and npcs do atmospheric things while players are around.   //
// See /doc/build/monsters                                                  //
//////////////////////////////////////////////////////////////////////////////

// Preprocessor //////////////////////////////////////////////////////////////
#pragma strict_types

// Variables /////////////////////////////////////////////////////////////////

/* private */  string* chat_head; // The actual chats
/* private */  int chat_chance;   // Percentage chance of chatting per heartbeat
/* private */  int regular_chat;  // Current position in chat cycle
/* private */  status regular;    // Are chats regular?

// Prototypes ////////////////////////////////////////////////////////////////

void chat();

// Code //////////////////////////////////////////////////////////////////////

void heart_beat()
{
  // Determine if we should chat.
  if (random(100) < chat_chance)
    chat();
}

// Name: chat
// Description: This causes the NPC to execute a chat.
void chat()
{
  string chat_string, lang;

  if(!chat_head || !sizeof(chat_head))
    return;
  if (regular) {
    chat_string = chat_head[ regular_chat ];
    if ( ++regular_chat >= sizeof( chat_head ) )
      regular_chat = 0;
  } else {
    chat_string = chat_head[random(sizeof(chat_head))];
  }
  if ( ('#' == chat_string[0]) && ('#' == chat_string[1]) )
    call_other( this_object(), chat_string[2..] );
  else if ( 2 == sscanf( chat_string, "@%s@%s", lang, chat_string ) )
    this_object()->language_say( chat_string, lang );
  else
    tell_room( environment(), get_f_string( chat_string ) );
}

// Name: load_chat
// Desctiption: Sets variables associated with chats.
void load_chat(int chance, string *strs, status flag)
{
  chat_head = strs;
  chat_chance = chance;
  regular = flag;
  regular_chat = 0;
}

// Chat Contents

string *query_chats()
{
  return chat_head;
}

void set_chats(string *chats)
{
  chat_head = chats;
  regular_chat = 0;
}

// Chat Chance

void set_chat_chance(int chance)
{
  chat_chance = chance;
}

int query_chat_chance()
{
  return chat_chance;
}

// Regular Chats

void set_regular_chats(status reg)
{
  regular = reg;
}

status query_regular_chats()
{
  return regular;
}

// EOF ///////////////////////////////////////////////////////////////////////
