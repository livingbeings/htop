/*
htop - RichString.c
(C) 2004,2011 Hisham H. Muhammad
Released under the GNU GPLv2, see the COPYING file
in the source distribution for its full text.
*/

#include "RichString.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "Macros.h"
#include "XUtils.h"


#define charBytes(n) (sizeof(CharType) * (n))

static void RichString_extendLen(RichString* this, int len) {
   if (this->chlen <= RICHSTRING_MAXLEN) {
      if (len > RICHSTRING_MAXLEN) {
         this->chptr = xMalloc(charBytes(len + 1));
         memcpy(this->chptr, this->chstr, charBytes(this->chlen));
      }
   } else {
      if (len <= RICHSTRING_MAXLEN) {
         memcpy(this->chstr, this->chptr, charBytes(len));
         free(this->chptr);
         this->chptr = this->chstr;
      } else {
         this->chptr = xRealloc(this->chptr, charBytes(len + 1));
      }
   }

   RichString_setChar(this, len, 0);
   this->chlen = len;
}

static void RichString_setLen(RichString* this, int len) {
   if (len < RICHSTRING_MAXLEN && this->chlen < RICHSTRING_MAXLEN) {
      RichString_setChar(this, len, 0);
      this->chlen = len;
   } else {
      RichString_extendLen(this, len);
   }
}

#ifdef HAVE_LIBNCURSESW

static inline int RichString_writeFromWide(RichString* this, int attrs, const char* data_c, int from, int len) {
   wchar_t data[len + 1];
   len = mbstowcs(data, data_c, len);
   if (len <= 0)
      return 0;

   int newLen = from + len;
   RichString_setLen(this, newLen);
   for (int i = from, j = 0; i < newLen; i++, j++) {
      this->chptr[i] = (CharType) { .attr = attrs & 0xffffff, .chars = { (iswprint(data[j]) ? data[j] : '?') } };
   }

   return wcswidth(data, len);
}

static inline int RichString_writeFromAscii(RichString* this, int attrs, const char* data, int from, int len) {
   int newLen = from + len;
   RichString_setLen(this, newLen);
   for (int i = from, j = 0; i < newLen; i++, j++) {
      this->chptr[i] = (CharType) { .attr = attrs & 0xffffff, .chars = { (isprint(data[j]) ? data[j] : '?') } };
   }

   return len;
}

inline void RichString_setAttrn(RichString* this, int attrs, int start, int charcount) {
   int end = CLAMP(start + charcount, 0, this->chlen);
   for (int i = start; i < end; i++) {
      this->chptr[i].attr = attrs;
   }
}

void RichString_appendChr(RichString* this, int attrs, char c, int count) {
   int from = this->chlen;
   int newLen = from + count;
   RichString_setLen(this, newLen);
   for (int i = from; i < newLen; i++) {
      this->chptr[i] = (CharType) { .attr = attrs, .chars = { c, 0 } };
   }
}

int RichString_findChar(RichString* this, char c, int start) {
   wchar_t wc = btowc(c);
   cchar_t* ch = this->chptr + start;
   for (int i = start; i < this->chlen; i++) {
      if (ch->chars[0] == wc)
         return i;
      ch++;
   }
   return -1;
}

#else /* HAVE_LIBNCURSESW */

static inline int RichString_writeFromWide(RichString* this, int attrs, const char* data_c, int from, int len) {
   int newLen = from + len;
   RichString_setLen(this, newLen);
   for (int i = from, j = 0; i < newLen; i++, j++) {
      this->chptr[i] = (((unsigned char)data_c[j]) >= 32 ? ((unsigned char)data_c[j]) : '?') | attrs;
   }
   this->chptr[newLen] = 0;

   return len;
}

static inline int RichString_writeFromAscii(RichString* this, int attrs, const char* data_c, int from, int len) {
   return RichString_writeFromWide(this, attrs, data_c, from, len);
}

void RichString_setAttrn(RichString* this, int attrs, int start, int charcount) {
   int end = CLAMP(start + charcount, 0, this->chlen);
   for (int i = start; i < end; i++) {
      this->chptr[i] = (this->chptr[i] & 0xff) | attrs;
   }
}

void RichString_appendChr(RichString* this, int attrs, char c, int count) {
   int from = this->chlen;
   int newLen = from + count;
   RichString_setLen(this, newLen);
   for (int i = from; i < newLen; i++) {
      this->chptr[i] = c | attrs;
   }
}

int RichString_findChar(RichString* this, char c, int start) {
   chtype* ch = this->chptr + start;
   for (int i = start; i < this->chlen; i++) {
      if ((*ch & 0xff) == (chtype) c)
         return i;
      ch++;
   }
   return -1;
}

#endif /* HAVE_LIBNCURSESW */

void RichString_prune(RichString* this) {
   if (this->chlen > RICHSTRING_MAXLEN)
      free(this->chptr);
   memset(this, 0, sizeof(RichString));
   this->chptr = this->chstr;
}

void RichString_setAttr(RichString* this, int attrs) {
   RichString_setAttrn(this, attrs, 0, this->chlen);
}

int RichString_appendWide(RichString* this, int attrs, const char* data) {
   return RichString_writeFromWide(this, attrs, data, this->chlen, strlen(data));
}

int RichString_appendnWide(RichString* this, int attrs, const char* data, int len) {
   return RichString_writeFromWide(this, attrs, data, this->chlen, len);
}

int RichString_writeWide(RichString* this, int attrs, const char* data) {
   return RichString_writeFromWide(this, attrs, data, 0, strlen(data));
}

int RichString_appendAscii(RichString* this, int attrs, const char* data) {
   return RichString_writeFromAscii(this, attrs, data, this->chlen, strlen(data));
}

int RichString_appendnAscii(RichString* this, int attrs, const char* data, int len) {
   return RichString_writeFromAscii(this, attrs, data, this->chlen, len);
}

int RichString_writeAscii(RichString* this, int attrs, const char* data) {
   return RichString_writeFromAscii(this, attrs, data, 0, strlen(data));
}
