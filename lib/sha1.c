#include "sha1.h"

char *sha1( char *val ){
   int msg_length = strlen( val );
   int hash_length = gcry_md_get_algo_dlen( GCRY_MD_SHA1 );
   unsigned char hash[ hash_length ];
   char *out = (char *) malloc( sizeof(char) * ((hash_length*2)+1) );
   char *p = out;
   gcry_md_hash_buffer( GCRY_MD_SHA1, hash, val, msg_length );
   int i;
   for ( i = 0; i < hash_length; i++, p += 2 ) {
      snprintf ( p, 3, "%02x", hash[i] );
   }
   return out;
}

