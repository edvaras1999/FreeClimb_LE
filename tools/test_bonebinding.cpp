// Compara o ciclo antigo (99 x n) com MapBonesByName (1 x n): mesmo resultado, muito menos ReadableMem.
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <vector>
#define _strnicmp strncasecmp
static unsigned long g_calls = 0;
static bool ReadableMem(unsigned a, unsigned s) { ++g_calls; return a >= 0x10000 && a + s >= a; }
static std::vector<std::string> g_names;
static const char * FC_BoneName(int b) { return g_names[b].c_str(); }

static unsigned MapBonesByName(unsigned entries, unsigned n, int * map)
{
	static const int engineOwned[] = { 0, 1, 2, 3, 33, 34, 42, 43, 60, 61, 62, 63, 64, 65, 66, 97, 98 };
	const char * names[99]; unsigned lens[99]; bool want[99];
	for(int b = 0; b < 99; ++b)
	{
		map[b] = -1; want[b] = true;
		for(size_t k = 0; k < sizeof(engineOwned) / sizeof(engineOwned[0]); ++k) if(engineOwned[k] == b) want[b] = false;
		names[b] = FC_BoneName(b); lens[b] = names[b] ? (unsigned)strlen(names[b]) : 0;
		if(!names[b] || lens[b] == 0 || lens[b] > 47) want[b] = false;
	}
	if(!ReadableMem(entries, n * 0x78)) return 0;
	unsigned bound = 0;
	for(unsigned i = 0; i < n; ++i)
	{
		const unsigned chars = *(const unsigned *)(entries + i * 0x78 + 0x74);
		if(!chars || !ReadableMem(chars, 48)) continue;
		const char * s = (const char *)chars;
		for(int b = 0; b < 99; ++b)
		{
			if(!want[b]) continue;
			if(_strnicmp(s, names[b], 47) == 0 && s[lens[b]] == 0) { map[b] = (int)i; want[b] = false; ++bound; }
		}
	}
	return bound;
}
static void OldMap(unsigned entries, unsigned n, int * map)
{
	const int engineOwned[] = { 0, 1, 2, 3, 33, 34, 42, 43, 60, 61, 62, 63, 64, 65, 66, 97, 98 };
	for(int bone = 0; bone < 99; ++bone)
	{
		map[bone] = -1; bool skip = false;
		for(size_t k = 0; k < sizeof(engineOwned) / sizeof(engineOwned[0]); ++k) if(engineOwned[k] == bone) skip = true;
		if(skip) continue;
		const char * name = FC_BoneName(bone);
		for(unsigned i = 0; i < n; ++i)
		{
			const unsigned e = entries + i * 0x78;
			if(!ReadableMem(e + 0x74, 4)) continue;
			const unsigned chars = *(const unsigned *)(e + 0x74);
			if(!chars || !ReadableMem(chars, 48)) continue;
			if(_strnicmp((const char *)chars, name, 47) == 0 && ((const char *)chars)[strlen(name)] == 0) { map[bone] = (int)i; break; }
		}
	}
}
int main()
{
	srand(7);
	for(int b = 0; b < 99; ++b) { char t[48]; snprintf(t, sizeof t, "NPC Bone%d [B%d]", b, b); g_names.push_back(t); }
	const unsigned n = 642;
	char * mem = (char *)mmap(0, 1 << 20, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
	char * strs = mem + n * 0x78 + 4096;
	int bad = 0;
	for(int round = 0; round < 50; ++round)
	{
		memset(mem, 0, 1 << 20);
		char * sp = strs;
		for(unsigned i = 0; i < n; ++i)
		{
			const int r = rand() % 100;
			std::string name;
			if(r < 40) { name = g_names[rand() % 99]; if(rand() % 3 == 0) for(auto & c : name) c = toupper(c); }	// osso canonico (por vezes em maiusculas)
			else if(r < 45) name = "";						// sem nome
			else { name = "Extra" + std::to_string(rand() % 1000); }
			if(r < 45 && name.empty()) { *(unsigned *)(mem + i * 0x78 + 0x74) = 0; continue; }
			strcpy(sp, name.c_str()); *(unsigned *)(mem + i * 0x78 + 0x74) = (unsigned)(size_t)sp; sp += name.size() + 1;
		}
		int a[99], b[99];
		g_calls = 0; OldMap((unsigned)(size_t)mem, n, a); unsigned oldCalls = g_calls;
		g_calls = 0; unsigned bound = MapBonesByName((unsigned)(size_t)mem, n, b); unsigned newCalls = g_calls;
		unsigned cmp = 0; for(int k = 0; k < 99; ++k) { if(a[k] != b[k]) ++bad; if(b[k] >= 0) ++cmp; }
		if(round == 0) printf("round 0: ReadableMem calls  old=%u  new=%u  (bound=%u, same=%s)\n", oldCalls, newCalls, bound, cmp == bound ? "yes" : "NO");
	}
	printf("%s: %d differences over 50 random skeletons of %u entries\n", bad ? "FAIL" : "OK", bad, n);
	return bad != 0;
}
