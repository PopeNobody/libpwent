#include "config.h"
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
#include <ctype.h>

static struct passwd passwds[1024];
static struct group groups[1024];
static struct servent servents[1024];

const size_t BUF_SIZE=1024;

static int pw_pos=-1;
static int gr_pos=-1;
static int s_pos=-1;

const static void *mmap_bad=(void*)(unsigned long)-1;
struct gai_error {
  int code;
  const char *msg;
};
#define ERROR(x) { x, #x }

struct gai_error gai_errors[] = {
  ERROR(EAI_FAMILY),
#ifdef EAI_ADDRFAMILY
  ERROR(EAI_ADDRFAMILY),
#endif
  ERROR(EAI_AGAIN),
  ERROR(EAI_BADFLAGS),
  ERROR(EAI_FAIL),
  ERROR(EAI_FAMILY),
  ERROR(EAI_MEMORY),
#ifdef EAI_NODATA
  ERROR(EAI_NODATA),
#endif
  ERROR(EAI_NONAME),
  ERROR(EAI_SERVICE),
  ERROR(EAI_SOCKTYPE),
  ERROR(EAI_SYSTEM),
  {0,0}
};

#define countof(x) sizeof(x)/sizeof(x[0])
static char *next_str(char **pp);

static int count(const char *beg, const char *end, char val){
  int count=0;
  while(beg!=end){
    if(*beg++ == val)
      ++count;
  };
  return count;
};
static char **service_lines(char * const beg, char * const end){
  int nlines = count(beg,end,'\n');
  char **lines = (char**)malloc(sizeof(char*)*(nlines+2));
  memset(lines,0,sizeof(lines));
  int line=0;
  char *pos=beg;
  while(pos!=end) {
    if(pos!=end)
      lines[line++]=pos;
    while(pos!=end && *pos!='\n') {
      if(*pos=='#')
        *pos=0;
      pos++;
    }
    if(pos!=end)
      *pos++=0;
  };
  while(line<=nlines)
    lines[line++]=0;
  int d=0;
  for(int i=0;i<nlines;i++){
    if(lines[i][0])
      lines[d++]=lines[i];
  }
  while(lines[d])
    lines[d++]=0;
  return lines;
}
char dummy[1]={0};
char *pdummy=&dummy[0];

static char**split_words(char *beg, char *end){
  static char*words[1024];
  memset(&words,0,sizeof(words));
  int word=0;
  while(beg!=end) {
    while(beg!=end && isspace(*beg))
      beg++;
    if(beg==end)
      break;
    words[word++]=beg;
    while(beg!=end && !isspace(*beg))
      beg++;
    if(beg!=end)
      *beg++=0;
  };
  words[word++]=0;
  return words;
}
static int count_words(char **words){
  int count=0;
  while(words[count++])
    ;
  return count;
};
static struct servent parse_servent(char * const beg){
  assert(*beg);
  char * const end = beg+strlen(beg);
  assert(!*end);
  char *pos=beg;
  while(pos!=end && isspace(*pos))
    pos++;
  struct servent servent;
  memset(&servent,0,sizeof(servent));
  char **words=split_words(beg,end);
  if(*words)
   servent.s_name=*words++;
  if(*words)
   servent.s_proto=*words++;
  servent.s_port=atoi(servent.s_proto);
  while(*servent.s_proto && *servent.s_proto!='/')
    servent.s_proto++;
  if(*servent.s_proto)
    servent.s_proto++;

  int count=count_words(words);
  servent.s_aliases = malloc(sizeof(char*)+sizeof(char*)*count);
  for(int i=0;i<count;i++){
    servent.s_aliases[i]=words[i];
  };
  words[count]=0;
  return servent;
};
struct group *getgrnam(const char *name) {
  static struct group group;
  static char buf[16*1024];
  static struct group *result;
  int res = getgrnam_r(name, &group, buf,sizeof(buf), &result);
  if(res)
    return 0;
  return result;
};

struct group *getgrgid(gid_t gid) {
  static struct group group;
  static char buf[16*1024];
  static struct group *result;
  int res = getgrgid_r(gid, &group, buf,sizeof(buf), &result);
  if(res)
    return 0;
  return result;
};

int getgrnam_r(const char *name, struct group *grp,
    char *buf, size_t buflen, struct group **result)
{
  *result=0;
  return 0;
};

int getgrgid_r(gid_t gid, struct group *grp,
    char *buf, size_t buflen, struct group **result)
{
  *result=0;
  return 0;
};
struct group *getgrent(void) {
  if(gr_pos<0)
    setgrent();    
  if(groups[gr_pos].gr_name==0) {
    endgrent();
    return 0;
  }
  return groups+gr_pos++;
};
void finish_group(struct group *group){
  char *p=group->gr_name;
  group->gr_passwd=next_str(&p);
  char *str_gid=next_str(&p);
  char *str_mem=next_str(&p);
  group->gr_gid=atoi(str_gid);
  int count=0;
  int i;
  for(i=0;str_mem[i];i++){
    if(str_mem[i]==',')
      count++;
  };
  int length=i;
  group->gr_mem=malloc(sizeof(char*)*count+sizeof(char*));
  count=0;
  for(i=0;i<length;i++){
    group->gr_mem[count++]=str_mem+i;
    while(i<length && str_mem[i]!=',')
      i++;
    if(i<length)
      str_mem[i++]=0;
  };
  group->gr_mem[count++]=0;
};
void setgrent(void){
  static volatile int busy=0;
  while(busy) {
    dprintf(2,"waiting in setgrent");
    sleep(1);
  };
  busy=1;
  assert(busy);
  if(!groups[0].gr_name){
    memset(groups,0,sizeof(groups));

    int fd=open("/etc/group",O_RDONLY);
    if(fd<0)
      goto done;
    size_t size=lseek(fd,0,SEEK_END);
    const char * const b =mmap(0,size,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
    close(fd);
    if(mmap_bad==b)
      goto done;
    const char *e=b+size;
    *(char*)e=0;
    assert(strlen(b)==size);
    const char *p=b;
    struct group group_0;
    struct group group;
    int i=0;
    for(char *p=(char*)b;p<e;){
      const char *l=p;
      group.gr_name=p;
      while(p!=e && *p!='\n')
        ++p;
      if(!*p)
        break;
      *p++=0;
      finish_group(&group);
      if(i==0){
        group_0=group;
      } else {
        groups[i]=group;
      }
      ++i;
    };
    groups[0]=group_0;
  }
  endgrent();
done:
  busy=0;
};

void endgrent(void){
  gr_pos=0;
};

/* Utility Routines */
static char *next_str(char **pp){
  char *p=*pp;
  while(*p && *p!=':')
    ++p;
  if(*p)
    *p++=0;
  *pp=p;
  return p;
};

/* passwd routines */

struct passwd *getpwuid(uid_t uid) {
  static struct passwd passwd;
  static char buf[16*1024];
  static struct passwd *result;
  int res = getpwuid_r(uid, &passwd, buf,sizeof(buf), &result);
  if(res)
    return 0;
  return result;
};
struct passwd *getpwnam(const char *name) {
  static struct passwd passwd;
  static char buf[16*1024];
  static struct passwd *result;
  int res = getpwnam_r(name, &passwd, buf,sizeof(buf), &result);
  if(res)
    return 0;
  return result;
};

size_t min(size_t lhs, size_t rhs){
  if(lhs<rhs)
    return lhs;
  else
    return rhs;
};

static int getpw_r(const struct passwd *ent, struct passwd *pwd,
    char *buf, size_t buflen, struct passwd **result)
{
  if(ent){
#define copy_str(mem) { char *src=ent->mem; pwd->mem=dst; while(*src){ *dst++=*src++; } *dst++=*src++; }
    char *dst=buf;
    copy_str(pw_name);
    copy_str(pw_gecos);
    copy_str(pw_dir);
    copy_str(pw_shell);
    pwd->pw_gid=ent->pw_gid;
    pwd->pw_uid=ent->pw_uid;
    *result=pwd;
  };
  return 0;
};
int getpwnam_r(const char *name, struct passwd *pwd,
    char *buf, size_t buflen, struct passwd **result)
{
  memset(buf,0,buflen);
  memset(pwd,0,sizeof(*pwd));
  *result=0;

  struct passwd *ent=0;
  while(1){
    ent = getpwent();
    if(!ent)
      break;
    if(!strcmp(name,ent->pw_name))
     break;
  };
  return getpw_r(ent,pwd,buf,buflen,result);
}

int getpwuid_r(uid_t uid, struct passwd *pwd,
    char *buf, size_t buflen, struct passwd **result)
{
  memset(buf,0,buflen);
  memset(pwd,0,sizeof(*pwd));
  *result=0;

  struct passwd *ent=0;
  while(1){
    ent = getpwent();
    if(!ent)
      break;
    if(uid==ent->pw_uid)
     break;
  };
  return getpw_r(ent,pwd,buf,buflen,result);
};
static void finish_passwd(struct passwd *pwd)
{
  assert(pwd);
  char *p=pwd->pw_name;
  pwd->pw_passwd=next_str(&p);
  char *str_uid=next_str(&p);
  pwd->pw_uid=atoi(str_uid);
  char *str_gid=next_str(&p);
  pwd->pw_gid=atoi(str_gid);
  pwd->pw_gecos=next_str(&p);
  pwd->pw_dir=next_str(&p);
  pwd->pw_shell=next_str(&p);
};
void setpwent()
{
  static volatile int busy=0;
  while(busy) {
    dprintf(2,"waiting in setpwent");
    sleep(1);
  };
  busy=1;
  assert(busy);
  if(!passwds[0].pw_name){
    memset(passwds,0,sizeof(passwds));

    int fd=open("/etc/passwd",O_RDONLY);
    if(fd<0)
      goto done;
    size_t size=lseek(fd,0,SEEK_END);
    const char * const b =mmap(0,size+16*1024,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
    close(fd);
    if(mmap_bad==b)
      goto done;
    const char *e=b+size;
    *(char*)e=0;
    assert(strlen(b)==size);
    const char *p=b;
    struct passwd passwd_0;
    struct passwd passwd;
    int i=0;
    for(char *p=(char*)b;p<e;){
      const char *l=p;
      passwd.pw_name=p;
      while(p!=e && *p!='\n')
        ++p;
      if(!*p)
        break;
      *p++=0;
      finish_passwd(&passwd);
      if(i==0){
        passwd_0=passwd;
      } else {
        passwds[i]=passwd;
      }
      ++i;
    };
    passwds[0]=passwd_0;
  }
  endpwent();
done:
  busy=0;
};

void endpwent(void)
{
  pw_pos=0;
};

struct passwd *getpwent(void) {
  if(pw_pos<0)
    setpwent();    
  if(passwds[pw_pos].pw_name==0) {
    endpwent();
    return 0;
  }
  return passwds+pw_pos++;
};



struct servent *getservent(void)
{
  if(s_pos<0)
    setservent(1);
  if(servents[s_pos].s_name)
    return servents+s_pos++;
  return 0;
};
static void finish_servent(struct servent *ent){
  char *p=ent->s_name;
  int count=0;
  int space=0;
  while(*p && *p!='#') {
    if(isspace(*p)) {
      if(!space)
        count++;
      space=1;
      *p=0;
    } else {
      space=0;
    };
    p++;
  };
  char *e=p;
  *p=0;
};
void setservent(int stayopen)
{
  static volatile int busy=0;
  while(busy) {
    dprintf(2,"waiting in setservent");
    sleep(1);
  };
  busy=1;
  assert(busy);
  if(!servents[0].s_name){
    memset(servents,0,sizeof(servents));

    int fd=open("/etc/services",O_RDONLY);
    if(fd<0)
      goto done;
    size_t size=lseek(fd,0,SEEK_END);
    char * const b =mmap(0,size,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
    close(fd);
    if(mmap_bad==b)
      goto done;
    char *e=b+size;
    *(char*)e=0;
    assert(strlen(b)==size);
    struct servent servent_0;
    struct servent servent;
    memset(&servent,0,sizeof(servent));
    memset(&servent_0,0,sizeof(servent_0));
    if(1){
      char **lines=service_lines(b,e);
      for(int i=0;lines[i];i++) {
        servent=parse_servent(lines[i]);
        if(i){
          servents[i]=servent;
        } else {
          servent_0=servent;
        }
      } 
    }
    servents[0]=servent_0;
  }
  endservent();
done:
  busy=0;
};

void endservent(void)
{
  s_pos=0;
};

struct servent *getservbyname(const char *name, const char *proto)
{
  return 0;
};

struct servent *getservbyport(int port, const char *proto)
{
  return 0;
};

int getaddrinfo(const char *node, const char *service,
    const struct addrinfo *hints,
    struct addrinfo **res)
{
  return EAI_FAIL;
};

void freeaddrinfo(struct addrinfo *res)
{
};

const char *gai_strerror(int errcode)
{
  for(int i=0;i<countof(gai_errors);i++){
    if(gai_errors[i].code==errcode)
      return gai_errors[i].msg;
  };
  return "unknown error";
};
#ifdef WITHMAIN
static void dump_servent(struct servent *servent)
{
  dprintf(1,"%s %d %s", servent->s_name, servent->s_port, servent->s_proto);
  char **aliases=servent->s_aliases;
  for(int i=0;aliases[i];i++)
    dprintf(1," %s", aliases[i]);
  dprintf(1,"\n");
};
int main(int argc, char**argv){
  setservent(0);
  struct servent *ent = getservent();
  while(ent){
    dump_servent(ent);
    ent=getservent();
  };
  return 0;
};
#endif


