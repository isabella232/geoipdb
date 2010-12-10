#include "ipdb.h"

#include <search.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>


void cpy(char **from,char **to){
  *to = malloc(strlen(*from));
  strcpy(*to,*from);
}


void 
print_range(const IpRange* e){
  printf( "from: %lu, to:%lu ->City-idx: %i \n",e->from, e->to,e->city_index );    
}

void 
print_ranges(IPDB * db){
  int i;
  for(i = 0; i < db->ranges_count; ++i)
  {
    print_range(&(db->ranges[i]));
  }
}

void 
print_city(const City * e){
  printf( "City: code:%i, name:%s, country: %s, lat: %10.7f, lng: %10.7f  \n",e->city_code, e->name, e->country_iso3, e->lat, e->lng );    
}

void 
print_cities(IPDB * db){
  int i;
  for(i = 0; i < db->cities_count; ++i)
  {
    print_city(&(db->cities[i]));
  }
}



void print_stats(IPDB * db){
  printf("DB STATS: \n");
  printf("\tCities: %i\n", db->cities_count);
  printf("\tRanges: %i\n", db->ranges_count);  
}

double 
get_time(struct timeval *tim){
  gettimeofday(tim, NULL);
  return tim->tv_sec+(tim->tv_usec/1000000.0);
}


unsigned long
ip_to_int(const char *addr){
	unsigned int    c, octet, t;
	unsigned long   ipnum;
	int             i = 3;

	octet = ipnum = 0;
	while ((c = *addr++)) {
		if (c == '.') {
			if (octet > 255)
				return 0;
			ipnum <<= 8;
			ipnum += octet;
			i--;
			octet = 0;
		} else {
			t = octet;
			octet <<= 3;
			octet += t;
			octet += t;
			c -= '0';
			if (c > 9)
				return 0;
			octet += c;
		}
	}
	if ((octet > 255) || (i != 0))
		return 0;
	ipnum <<= 8;
	return ipnum + octet;
}


// Function to compare 
// - either two ip-ranges: i.e.: a(from...to) <=> b(from...to)
// - or a ip(i.e. range without to) and an ip-range: i.e. a(from...NULL) <=> b(from...to); a(from...to) <=> b(from ... NULL)
int compare_ranges(const void *fa, const void *fb) {
  if(fa == NULL)
  {
    printf("FA IS NULL!!!\n");
    return 0;
  }
  if(fb == NULL)
  {
    printf("FB IS NULL!!!\n");
    return 0;
  }


  const IpRange *a = (IpRange *) fa;
  const IpRange *b = (IpRange *) fb;  

  // printf("\tComparing: a:");
  // print_range(a);
  // printf(" with b:");
  // print_range(b);
  // printf("\n");

  if(a->from>0 && a->to>0 && b->from>0 && b->to>0){ //regular case: both entries are ranges
    if(a->to  <  b->from)    {
      return -1;
    }else if(a->from > b->to){
      return +1;
    }else{
      return 0;
    }
  }else if(a->to == 0 && b->to>0){//a is a search_object
    if(a->from  <  b->from)    {
      return -1;
    }else if(a->from > b->to){
      return +1;
    }else{
      return 0;
    }
  }else if(b->to == 0 && a->to>0){//b is a search_object
    if(b->from  <  a->from){
      return -1;
    }else if(b->from > a->to){
      return +1;
    }else{
      return 0;
    }
  }else if(a->to == 0 && b->to == 0){ //both are search objects - this should not happen!
    return a->from - b->from;
  }
  return 0;
}


int 
compare_cities(const void *a, const void *b){
  const City city_a = *(City*)a;
  const City city_b = * (City*) b;
  // sort cities by city_code
  return city_a.city_code - city_b.city_code;
}

void
sort_cities(IPDB * db){
  printf("Sorting %i Cities in db...\n", db->cities_count);
  
  struct timeval tim;
  double t1 = get_time(&tim);  
  
  qsort(db->cities,db->cities_count,sizeof(City), compare_cities);  

  printf("\n Sorting cities needed %.6lf seconds\n", get_time(&tim)-t1);  
}


int // returns a city-inde
city_index_by_code(IPDB * db, uint16 city_code){
  City *search, *result;
  search = malloc(sizeof(City));
  search->city_code = city_code;
  result = (City*) bsearch(search, db->cities, db->cities_count, sizeof(City), compare_cities);      
  if(result == NULL)
  {
    printf("Could not find searched city with code: %i \n", city_code);
    return -1;
  }else{
    // printf("Found result: \t");
    // print_city(result);
    int index;
    index = (result - db->cities);
    // printf("Found index: %i\n", index);
    return index;
  }
}


City * 
city_by_ip(IPDB *db, char *ip){
  IpRange * search, *result;
  search = (IpRange *)malloc(sizeof(IpRange));
  result = (IpRange *)malloc(sizeof(IpRange));  
  void * res;

  // print_stats(db);
  
  if(db == NULL)
  {
    printf("ERROR: DB ist NULL! ");
    return NULL;
  }
  
  if(db->ranges_count == 0)
  {
    printf("ERROR: DB has no Ranges Data. Can not search!\n");
    return NULL;
  }
    
  search->from = ip_to_int(ip);
  search->to=0;
  search->city_index = 0;

  printf("Searching for: ip=%s, ipnum=%lu \n", ip, search->from);
  // result = (IpRange*) bsearch(search, db->ranges, db->ranges_count-2, sizeof(IpRange), compare_ranges);    
  res = bsearch(search, db->ranges, db->ranges_count, sizeof(IpRange), compare_ranges);

  if(res == NULL)
  {
    printf("Could not find the IP: %s! THIS SHOULD NOT HAPPEN!\n", ip);
    return NULL;
  }else{
    printf("Found Range: \t");
    result = (IpRange*) res;
    print_range(result);  
  }
  
  if(db->cities_count == 0)
  {
    printf("ERROR: DB has no City Data. Can not search!\n");
    return NULL;
  }
  
  
  if(result->city_index >0 && result->city_index < db->cities_count)
  {
    // address the city directly via the array-idx
    return &(db->cities[result->city_index]);
  } else {
    printf("Could not find city with index: %i - THIS SHOULD NOT HAPPEN!\n", result->city_index);
    return NULL; 
  }
}


// read ip-ranges from csv file, of format:
// from_ip|to_ip|city_code
void 
read_ranges_csv(IPDB * db){  
  struct timeval tim;
  double t1 = get_time(&tim);

  db->ranges = malloc(sizeof(IpRange) * db->max_ranges_count);
  
  printf("Parsing RANGES-CSV-file: %s\n", db->ranges_csv_file);
  FILE * f = fopen(db->ranges_csv_file, "rt");
  if(f == NULL)
  {
    printf("Could not open the CSV-file: %s", db->ranges_csv_file);
    return;
  }
  char line[256];
  char* from = NULL;
  char* to = NULL;
  char* city_code = NULL;
  int invalid_cities_count = 0;
  int city_index;
  
  IpRange* entry;
  db->ranges_count = 0;
  while (fgets(line,sizeof(line),f) && db->ranges_count < db->max_ranges_count){
    if(db->ranges_count % 100000 == 0)
      printf("Worked lines: %i\n", db->ranges_count);

    from =      strtok(line, RANGES_DELIM);
    to =        strtok(NULL, RANGES_DELIM);
    city_code = strtok(NULL, RANGES_DELIM);
    city_index = city_index_by_code(db, atoi(city_code));
    if(city_index < 0)
    {
      printf("Could not find city for code: %i", atoi(city_code));
      invalid_cities_count ++;
      continue;
    }else{
      entry = &(db->ranges[db->ranges_count]);
    
      entry->from = ip_to_int(from);
      entry->to = ip_to_int(to);
      // entry->city_code = atoi(city_code);
    
      entry->city_index = city_index;
      // printf("Line: %s", line);
      // printf("from: %u,to: %u, city_code:%i \n",entry->from,entry->to,entry->city_code);
      // db->ranges[i] = *entry;
      // printf("working record nr: %li\n", db->ranges_count);
      db->ranges_count++;
    }
  } 
  if(invalid_cities_count)
  {
    printf("Found invalid cities: %i", invalid_cities_count);
  }
  printf("\n Parsing of %i records needed %.6lf seconds\n", db->ranges_count, get_time(&tim)-t1);  
}


//read city-data from csv-file of format:
// COUNTRY,REGION,CITY-NAME,METRO-CODE,CITY-CODE,LATITUDE,LONGITUDE
void
read_cities_csv(IPDB * db){
  struct timeval tim;
  double t1 = get_time(&tim);

  db->cities_count = 0;
  db->cities = malloc(sizeof(City) * db->max_cities_count);
  
  printf("Parsing Cities-CSV-file: %s\n", db->cities_csv_file);
  FILE * f = fopen(db->cities_csv_file, "rt");
  if(f == NULL)
  {
    printf("Could not open the Cities-CSV-file: %s", db->cities_csv_file);
    return;
  }
  char line[256];
  char  *country, *region, *name,*metro_code,*city_code,*lat,*lng ;
  int i = 0;
  City* entry;

  while (fgets(line,sizeof(line),f) && db->cities_count < db->max_cities_count){
    i++;
    if(i == 1)
      continue;//skip the header

    if(DEBUG && i % 1000000 == 0)
    {
      printf("Worked lines: %i\n", i);
    }
   // printf("Line: %s", line);
   // COUNTRY,REGION,CITY-NAME,METRO-CODE,CITY-CODE,LATITUDE,LONGITUDE    
    country =    strtok(line, CITIES_DELIM);
    region =     strtok(NULL, CITIES_DELIM);
    name =       strtok(NULL, CITIES_DELIM);
    metro_code = strtok(NULL, CITIES_DELIM);    
    city_code =  strtok(NULL, CITIES_DELIM);
    lat =        strtok(NULL, CITIES_DELIM);
    lng =        strtok(NULL, CITIES_DELIM);    
    
    entry = &(db->cities[db->cities_count]);
    
    strncpy(entry->country_iso3, country, strlen(country));
    strncpy(entry->name, name, strlen(name));
        
    entry->city_code =    atoi(city_code);
    entry->lat =          atof(lat);
    entry->lng =          atof(lng);
    db->cities_count++;
  }
  if(DEBUG)
    printf("\n Parsing of %i records needed %.6lf seconds\n", db->cities_count, get_time(&tim)-t1);
}

/**
cache-file is an exact binary copy of the ranges+cities-arrays from memory,
the layout goes like this:
  db->cities_count [4 Bytes]
  db->ranges_count [4 Bytes]

  db->cities [sizeof(City)=24 x db->ranges_count Bytes]
  db->ranges [sizeof(IpRange)=24 x db->ranges_count Bytes]
*/
void  
write_cache_file(IPDB * db){
  struct timeval tim;
  double t1 = get_time(&tim);
  int objects_written;
  
  FILE * f;
  f = fopen(db->cache_file_name, "w");
  if(f==NULL){
    printf("Could not open Cache-File: %s", db->cache_file_name);
    return;
  }
  
  printf("Dumping %i records to cache-file: %s\n\n", db->ranges_count, db->cache_file_name);
  
  //write the record length at file header
  printf("Writing DB-Header of length: %li\n",sizeof(db->ranges_count));
  
  printf("RecordLength: %li\n",sizeof(IpRange));  
  printf("FieldLength: %li\n",sizeof(db->ranges[0].from));    
  
  //write the header: i.e.: numbers of records
  fwrite(&(db->cities_count), sizeof(db->cities_count),1,f);  
  fwrite(&(db->ranges_count), sizeof(db->ranges_count),1,f);
  

  printf("Writing Contents with %i cities, a %li bytes each, should = %li \n", db->cities_count, sizeof(City), db->cities_count * sizeof(City));  
  //write the actual data: all the ranges-array-buffer:
  objects_written = fwrite(db->cities, sizeof(City), db->cities_count, f);

  printf("Writing Contents with %i ranges, a %li bytes each, should = %li \n", db->ranges_count, sizeof(IpRange), db->ranges_count * sizeof(IpRange));  
  //write the actual data: all the ranges-array-buffer:
  objects_written += fwrite(db->ranges, sizeof(IpRange), db->ranges_count, f);
  

  fclose(f);
  
  printf("\n Writing CacheFile of %i objects needed %.6lf seconds\n", objects_written, get_time(&tim)-t1);  
}

int 
read_cache_file(IPDB * db){ 
  struct timeval tim;
  double t1 = get_time(&tim);  
  FILE * f;
  f = fopen(db->cache_file_name, "r");
  if(f==NULL){
    printf("Could not open Cache-File: %s", db->cache_file_name);
    return 0;
  }
  int cities_header_read = fread(&(db->cities_count), sizeof(db->cities_count),1,f);
  int ranges_header_read = fread(&(db->ranges_count), sizeof(db->ranges_count),1,f);

  
  if(cities_header_read == 0 || ranges_header_read == 0 || db->cities_count == 0 || db->ranges_count ==0 )
  {
    printf("Could not read Cities-Header from Cache-File: %s", db->cache_file_name);
    return 0;
  }
  
  printf("Reading DB-Header from Cache-File: %s, with %i cities and %i ranges\n",db->cache_file_name, db->cities_count, db->ranges_count);  

  int objects_read = 0;

  printf("Allocating: %lu for cities-array \n", sizeof(City)*(db->cities_count));
  db->cities = malloc(sizeof(City) * db->cities_count);
  objects_read += fread(db->cities, sizeof(City),db->cities_count,f);

  printf("Allocating: %lu for ranges-array \n", sizeof(IpRange)*(db->ranges_count));
  db->ranges = malloc(sizeof(IpRange) * db->ranges_count);
  objects_read += fread(db->ranges, sizeof(IpRange),db->ranges_count,f);
  

  fclose(f);
  printf("Reading cacheFile of %i objects needed %.6lf seconds\n", objects_read, get_time(&tim)-t1);    
  return objects_read;
}

void 
benchmark_search(IPDB * db,int count){
  printf("Benchmarking City-Search-Function with %i counts \n", count);
  struct timeval tim;  
  double t1 = get_time(&tim);
  int i;
  City * city;
  for(i=0;i<count; i++){

     city = city_by_ip(db, "278.50.47.0");
  }
  printf("\n\nSearch: %.6lf seconds elapsed\n", get_time(&tim)-t1);  
}

IPDB * init_db(char * cities_csv_file, char * ranges_csv_file, char * cache_file_name){
  IPDB *db;
  db = (IPDB*)malloc(sizeof(IPDB));
  if (db == NULL) //no memory left
    return NULL;
  db->cache_file_name = cache_file_name;

  db->cities_csv_file = cities_csv_file;
  db->max_cities_count = MAX_CITIES_COUNT;
  db->ranges_csv_file = ranges_csv_file;
  db->max_ranges_count = MAX_RANGES_COUNT;
    

  if(USE_CACHE && read_cache_file(db) > 0){
    if(DEBUG)
      printf("Loaded DB from Cache-File with %i records \n", db->ranges_count);
  }else{
    if(DEBUG)
      printf("Initializing IPDB from CSV-file: %s \n", db->ranges_csv_file);    
    read_cities_csv(db);
    // print_cities(db);
    if(db->cities_count == 0)
    {
      return NULL;
    }
    sort_cities(db);
    read_ranges_csv(db);
    // //TODO: sort ranges
    if(db!=NULL && db->ranges_count > 0 && USE_CACHE)
    {
      if(DEBUG)
        printf("Got %i records from CSV-file, writing to cache...\n", db->ranges_count);    
      write_cache_file(db);
    }
  }
  return db;  
}
