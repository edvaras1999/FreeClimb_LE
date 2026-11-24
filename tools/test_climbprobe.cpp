// Teste de ClimbProbe.h contra um mundo simulado de caixas. Compilar e correr:
//   g++ -std=c++14 -I../freeclimb-le test_climbprobe.cpp -o test_climbprobe && ./test_climbprobe
#include <stdio.h>
#include <vector>
#include <limits>
#include "ClimbProbe.h"
using namespace ClimbProbe;

struct Box { float x0, x1, y0, y1, z0, z1; };

// Raio contra caixas (so faces de entrada, como o Havok com malhas: quem comeca por dentro nao acerta nessa caixa).
struct World
{
	std::vector<Box> boxes;
	Hit operator()(float sx, float sy, float sz, float dx, float dy, float dz, float maxDist)
	{
		Hit best; best.hit = false; best.dist = maxDist; best.x = best.y = best.z = best.nx = best.ny = best.nz = 0;
		float bestT = maxDist;
		for(const Box & b : boxes)
		{
			const float o[3] = { sx, sy, sz }, d[3] = { dx, dy, dz };
			const float lo[3] = { b.x0, b.y0, b.z0 }, hi[3] = { b.x1, b.y1, b.z1 };
			bool inside = true;
			for(int a = 0; a < 3; ++a) if(o[a] <= lo[a] || o[a] >= hi[a]) inside = false;
			if(inside) continue;
			float tmin = -1e30f, tmax = 1e30f; int axis = -1; float sign = 0;
			bool miss = false;
			for(int a = 0; a < 3 && !miss; ++a)
			{
				if(fabsf(d[a]) < 1e-9f) { if(o[a] < lo[a] || o[a] > hi[a]) miss = true; continue; }
				float t1 = (lo[a] - o[a]) / d[a], t2 = (hi[a] - o[a]) / d[a];
				float s = -1.0f;			// entrou pela face 'lo': normal aponta para -eixo
				if(t1 > t2) { float t = t1; t1 = t2; t2 = t; s = 1.0f; }
				if(t1 > tmin) { tmin = t1; axis = a; sign = (d[a] > 0) ? -1.0f : 1.0f; (void)s; }
				if(t2 < tmax) tmax = t2;
			}
			if(miss || tmin > tmax || tmin < 0.0f || tmin > bestT) continue;
			bestT = tmin; best.hit = true; best.dist = tmin;
			best.x = sx + dx * tmin; best.y = sy + dy * tmin; best.z = sz + dz * tmin;
			best.nx = best.ny = best.nz = 0;
			if(axis == 0) best.nx = sign; else if(axis == 1) best.ny = sign; else best.nz = sign;
		}
		return best;
	}
};

static int g_fail = 0;
#define CHECK(c, msg) do { if(c) printf("  ok   %s\n", msg); else { printf("  FAIL %s\n", msg); ++g_fail; } } while(0)

int main()
{
	const float BIG = 5000.0f;
	// Jogador em x=-30 (gap 30), a olhar para +x; parede em x>=0.
	const float fx = 1.0f, fy = 0.0f, px = -30.0f, py = 0.0f;

	printf("ENTRADA\n");
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});			// parede alta
		const char * why = "";
		Wall r = ProbeEntryWall(w, fx, fy, px, py, 0.0f, 80.0f, 65.0f, &why);
		CHECK(r.found && fabsf(r.dist - 30.0f) < 0.01f, "parede alta: agarra");
	}
	{
		World w; w.boxes.push_back({0, 40, -30, 30, 0, 50});				// bau 50 de altura
		const char * why = "";
		Wall r = ProbeEntryWall(w, fx, fy, px, py, 0.0f, 80.0f, 65.0f, &why);
		CHECK(!r.found, "bau (50): nao agarra");
	}
	{
		World w; w.boxes.push_back({0, 60, -60, 60, 0, 100});				// pedra 100: abaixo de 86+24
		const char * why = "";
		Wall r = ProbeEntryWall(w, fx, fy, px, py, 0.0f, 80.0f, 65.0f, &why);
		CHECK(!r.found, "obstaculo de 100: nao agarra (fica para o parkour)");
	}
	{
		World w; w.boxes.push_back({0, 60, -60, 60, 0, 100});
		const char * why = "";
		Wall r = ProbeEntryWall(w, fx, fy, px, py, 0.0f, 50.0f, 65.0f, &why);
		CHECK(r.found, "MinLedge=50: o mesmo obstaculo de 100 ja agarra (o limite e respeitado)");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		w.boxes.push_back({-20, 0, -30, 30, 0, 45});						// bau encostado a parede
		const char * why = "";
		Wall r = ProbeEntryWall(w, fx, fy, px, py, 0.0f, 80.0f, 65.0f, &why);
		CHECK(r.found && fabsf(r.dist - 30.0f) < 0.01f, "bau junto a parede alta: a parede e a que conta");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 300, 600});			// so existe a 300 de altura
		const char * why = "";
		Wall r = ProbeEntryWall(w, fx, fy, px, py, 0.0f, 80.0f, 65.0f, &why);
		CHECK(!r.found, "superficie so em altura (nada ao nivel do peito do chao): nao agarra");
	}

	printf("CORPO LIVRE\n");
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		w.boxes.push_back({-25, 0, -50, 50, 10, 30});						// saliencia ao nivel dos pes, 25 de fundo
		float p = ClearancePush(w, fx, fy, px, py, 0.0f, 30.0f);
		CHECK(fabsf(p - 11.0f) < 0.01f, "saliencia aos pes (25): afasta 11");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		float p = ClearancePush(w, fx, fy, px, py, 0.0f, 30.0f);
		CHECK(p == 0.0f, "parede plana: nao afasta");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		w.boxes.push_back({-30, 0, -50, 50, 95, 105});					// saliencia fina a altura da cabeca (entre os raios antigos)
		float p = ClearancePush(w, fx, fy, px, py, 0.0f, 30.0f);
		CHECK(fabsf(p - 16.0f) < 0.01f, "saliencia fina a 100 (30 de fundo): afasta 16");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		w.boxes.push_back({-25, 0, -50, 50, 38, 44});					// saliencia fina a altura do joelho
		float p = ClearancePush(w, fx, fy, px, py, 0.0f, 30.0f);
		CHECK(fabsf(p - 11.0f) < 0.01f, "saliencia fina a 40: afasta 11");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		w.boxes.push_back({-200, 0, -50, 50, 100, 130});					// saliencia enorme a altura da cabeca
		float p = ClearancePush(w, fx, fy, px, py, 0.0f, 30.0f);
		CHECK(p == 0.0f || p <= 40.0f, "saliencia que ja envolve o jogador: limitado a 40");
	}

	printf("PAREDE RECUADA\n");
	{
		World w; w.boxes.push_back({90, 300, -BIG, BIG, -10, 900});			// a parede recuou 90
		Wall r = ProbeRecessedWall(w, -1.0f, 0.0f, px, py, 250.0f, 30.0f, 110.0f);
		CHECK(r.found && fabsf(r.dist - 120.0f) < 0.01f, "parede recuada 90: encontrada");
	}
	{
		World w; w.boxes.push_back({90, 300, -BIG, BIG, -10, 900});
		Wall r = ProbeRecessedWall(w, -1.0f, 0.0f, px, py, 250.0f, 30.0f, 50.0f);
		CHECK(!r.found, "recuou demais (alem do limite): larga");
	}
	{
		World w; w.boxes.push_back({-BIG, BIG, 60, 300, -10, 900});			// uma parede de lado (normal perpendicular)
		Wall r = ProbeRecessedWall(w, -1.0f, 0.0f, px, py, 250.0f, 30.0f, 110.0f);
		CHECK(!r.found, "outra parede (perpendicular) nao conta");
	}

	printf("BEIRAL\n");
	const float pz = 300.0f;				// pes; cabeca a 425
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-60, 0, -BIG, BIG, 430, 450});					// cornija 60 de fundo, 20 de espessura
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(o.ok, "cornija: planeia o salto");
		CHECK(o.ok && fabsf(o.destZ - 454.0f) < 0.01f, "  destino 4 acima do topo");
		CHECK(o.ok && fabsf(o.edgeOut - 30.0f) < 0.01f, "  bordo exterior a 30");
		CHECK(o.ok && o.wallAbove, "  parede continua por cima");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-400, 0, -BIG, BIG, 430, 450});					// laje enorme (piso)
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(!o.ok, "piso/teto largo: recusa");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-60, 0, -BIG, BIG, 430, 540});					// 110 de espessura
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(!o.ok, "saliencia grossa demais: recusa");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-60, 0, -BIG, BIG, 430, 450});
		w.boxes.push_back({-60, 0, -BIG, BIG, 480, 500});					// segunda cornija 30 acima: sem espaco
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(!o.ok, "duas cornijas coladas (sem espaco por cima): recusa");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-60, 0, -BIG, BIG, 520, 540});					// alta demais
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(!o.ok, "sem nada por cima da cabeca (cornija longe): recusa");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-60, 0, -BIG, BIG, 430, 450});
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 150.0f, 30.0f);
		CHECK(!o.ok, "maxRise=150: subida de 154 recusada");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(!o.ok, "parede lisa (nada por cima): recusa");
	}

	printf("BEIRAL RASO (a frente do corpo)\n");
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-25, 0, -BIG, BIG, 430, 450});					// 25 de fundo: o centro (a 30 da parede) fica de fora
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(o.ok, "beiral de 25 (nao cobre o centro): planeia o salto");
		CHECK(o.ok && fabsf(o.destZ - 454.0f) < 0.01f, "  destino 4 acima do topo");
		CHECK(o.ok && o.edgeOut < 0.0f, "  bordo exterior entre o centro e a parede");
	}
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, 0, 900});
		w.boxes.push_back({-4, 0, -BIG, BIG, 430, 450});					// 4 de fundo: nao chega ao corpo
		Overhang o = PlanOverhangHop(w, fx, fy, px, py, pz, 125.0f, 180.0f, 30.0f);
		CHECK(!o.ok, "friso de 4 (nao chega a frente do corpo): ignora");
	}

	printf("ESTABILIDADE (parado junto a um pilar fino que o raio ora acerta ora falha)\n");
	{
		// 5 s a 60 fps. O 'alvo' do afastamento alterna 0 / 25 / 0 / 25 ... (o raio acerta e falha), com ruido de +-1.5 na posicao medida.
		const float dt = 1.0f / 60.0f;
		unsigned seed = 12345;
		auto noise = [&]() { seed = seed * 1664525u + 1013904223u; return ((seed >> 8) & 0xFFFF) / 65535.0f * 3.0f - 1.5f; };

		// ANTIGO: afastamento = alvo, e posicao = o que o raio disse, todos os quadros
		float oldPush = 0, oldMin = 1e9f, oldMax = -1e9f, oldTravel = 0, oldPrev = 0;
		// NOVO: afastamento suavizado, e parado so corrige fora da zona morta
		float newPush = 0, newMin = 1e9f, newMax = -1e9f, newTravel = 0, newPrev = 0, pos = 30.0f;
		float oldPos = 30.0f, oldPosTravel = 0, newPosTravel = 0;
		for(int i = 0; i < 300; ++i)
		{
			const float target = ((i / 3) % 2 == 0) ? 0.0f : 25.0f;		// muda a cada 3 quadros
			// antigo
			oldPush = target;
			const float oldWanted = 30.0f + oldPush + noise();
			oldPosTravel += fabsf(oldWanted - oldPos); oldPos = oldWanted;
			if(oldPos < oldMin) oldMin = oldPos; if(oldPos > oldMax) oldMax = oldPos;
			oldTravel += fabsf(oldPush - oldPrev); oldPrev = oldPush;
			// novo (parado: o afastamento nao se recalcula, usa o ultimo)
			newPush = StepPush(newPush, 0.0f /* parado: mantem */ == 0.0f ? newPush : target, dt);
			const float wanted = 30.0f + newPush + noise();
			if(!HoldPosition(true, wanted - pos, 0.0f, 1.0f)) { newPosTravel += fabsf(wanted - pos); pos = wanted; }
			if(pos < newMin) newMin = pos; if(pos > newMax) newMax = pos;
			newTravel += fabsf(newPush - newPrev); newPrev = newPush;
		}
		printf("  antigo: posicao varia %.1f..%.1f, percurso total %.0f unidades\n", oldMin, oldMax, oldPosTravel);
		printf("  novo:   posicao varia %.1f..%.1f, percurso total %.0f unidades\n", newMin, newMax, newPosTravel);
		CHECK(oldMax - oldMin > 20.0f, "o comportamento antigo oscilava (amplitude > 20)");
		CHECK(newMax - newMin < 3.1f && newPosTravel < 1.0f, "parado, a posicao fica fixa (zona morta)");
	}
	{
		// em movimento: o afastamento sobe depressa e desce devagar, sem saltos
		float p = 0; bool ok = true; const float dt = 1.0f / 60.0f;
		for(int i = 0; i < 30; ++i) p = StepPush(p, 25.0f, dt);
		ok = ok && fabsf(p - 25.0f) < 0.01f;
		const float afterOne = StepPush(p, 0.0f, dt);
		ok = ok && (p - afterOne) < 1.6f;						// desce no maximo ~1,5 unidades por quadro (90/s)
		CHECK(ok, "afastamento: sobe depressa (25 em < 0,5 s), desce devagar (90/s)");
		CHECK(StepPush(10.0f, 10.0f, -1.0f) == 10.0f && StepPush(10.0f, 10.0f, 0.0f) == 10.0f, "dt invalido: nao muda");
		CHECK(!HoldPosition(false, 0.0f, 0.0f, 1.0f), "em movimento nunca fica parado");
		CHECK(!HoldPosition(true, 4.0f, 0.0f, 1.0f), "parado mas o erro passa da zona morta: corrige");
		CHECK(!HoldPosition(true, 0.0f, 0.0f, 0.9f), "parado mas a parede girou: corrige");
	}

	printf("ESTABILIDADE REAL (o mesmo caminho do jogo: trinco de 3 sondas + zona morta, sondas de 300 ms)\n");
	{
		// 20 s parado, 66 sondas de 0,3 s. mode 0: a medicao alterna ao acaso 0/25 (o pior caso: uma aresta no limite do raio)
		//                                 mode 1: sempre 25 (saliencia real)      mode 2: sempre 0
		auto run = [&](int mode, float * finalPush, int * changes, float * minGap)
		{
			unsigned seed = 99;
			StillPush st = { 0, 0.0f };
			float push = 0, pos = 30.0f; int n = 0; float lastChangeT = -10, minGapBetween = 1e9f;
			for(int probe = 0; probe < 66; ++probe)
			{
				seed = seed * 1664525u + 1013904223u;
				const float measured = mode == 0 ? (((seed >> 16) & 1) ? 25.0f : 0.0f) : (mode == 1 ? 25.0f : 0.0f);
				const float target = StillPushTarget(&st, measured, push, 0.3f);
				push = StepPush(push, target, 0.3f);
				const float wanted = 30.0f + push;
				if(!HoldPosition(true, wanted - pos, 0.0f, 1.0f))
				{
					const float t = probe * 0.3f;
					if(n > 0 && t - lastChangeT < minGapBetween) minGapBetween = t - lastChangeT;
					lastChangeT = t; ++n; pos = wanted;
				}
			}
			*finalPush = push; *changes = n; *minGap = minGapBetween;
		};
		float fp, mg; int ch;
		run(0, &fp, &ch, &mg);
		printf("  aresta que alterna (pior caso): %d mudancas de posicao em 20 s, a mais rapida a %.1f s da anterior\n", ch, ch > 1 ? mg : 0.0f);
		CHECK(ch <= 4 && (ch < 2 || mg >= 2.0f), "aresta que alterna: no maximo 4 mudancas lentas em 20 s (antes: dezenas por segundo)");
		run(1, &fp, &ch, &mg); CHECK(fabsf(fp - 25.0f) < 0.01f && ch == 1, "saliencia real (sempre 25): o corpo afasta-se UMA vez e fica la");
		run(2, &fp, &ch, &mg); CHECK(fp == 0.0f && ch == 0, "sem saliencia: push 0, nao se mexe");
		// o erro anterior: o push ficava congelado. Aqui: saliencia presente (push 25) e depois desaparece: tem de voltar a 0.
		StillPush st = { 0, 0.0f }; float push = 25.0f;
		for(int probe = 0; probe < 30; ++probe) { const float t = StillPushTarget(&st, 0.0f, push, 0.3f); push = StepPush(push, t, 0.3f); }
		CHECK(push == 0.0f, "a saliencia desaparece: o afastamento RELAXA para 0 (antes ficava congelado)");
		// medicao que difere pouco (< 6): nunca mexe
		StillPush s2 = { 0, 0.0f }; float p2 = 10.0f; bool moved = false;
		for(int probe = 0; probe < 30; ++probe) { const float t = StillPushTarget(&s2, (probe & 1) ? 14.0f : 6.0f, p2, 0.3f); if(t != p2) moved = true; }
		CHECK(!moved, "variacao de +-4 (dentro da zona morta): nunca mexe");
	}
	{
		// zona morta do afastamento (rocha irregular): bossas pequenas nao afastam o corpo
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		w.boxes.push_back({-8, 0, -50, 50, 95, 105});					// bossa de 8
		CHECK(ClearancePush(w, fx, fy, px, py, 0.0f, 30.0f, 18.0f, 30.0f) == 0.0f, "bossa de 8 (rocha): nao afasta");
		World v; v.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		v.boxes.push_back({-25, 0, -50, 50, 95, 105});					// saliencia de 25: 25 - 18 = 7
		CHECK(fabsf(ClearancePush(v, fx, fy, px, py, 0.0f, 30.0f, 18.0f, 30.0f) - 7.0f) < 0.01f, "saliencia de 25 (tolerancia 18): afasta 7");
		CHECK(ClearancePush(v, fx, fy, px, py, 0.0f, 30.0f, 18.0f, 5.0f) == 5.0f, "MaxBumpPush limita");
		CHECK(ClearancePush(v, fx, fy, px, py, 0.0f, 30.0f, 22.0f, 30.0f) == 0.0f, "tolerancia 22: a mesma saliencia (push 3) cai na zona morta");
	}

	printf("PONTO DE CHEGADA DA SUBIDA\n");
	{
		// rampa: a partir de x=0 (plano da parede) o chao sobe 'slope' por unidade, a partir de z=topo. Raios verticais apenas.
		struct SlopeWorld
		{
			float topAtWall, slope; bool parapet;
			Hit operator()(float sx, float sy, float sz, float dx, float dy, float dz, float maxDist)
			{
				Hit h; h.hit = false; h.dist = h.x = h.y = h.z = h.nx = h.ny = h.nz = 0;
				(void)sy; (void)dx; (void)dy;
				const float x = sx;													// x medido a partir do plano da parede (em sx = 0)
				if(dz < -0.5f && x > 0.0f)											// raio para baixo
				{
					const float z = topAtWall + slope * x, t = sz - z;
					if(t >= 0.0f && t <= maxDist) { const float n = sqrtf(1 + slope * slope); h.hit = true; h.dist = t; h.x = sx; h.z = z; h.nx = -slope / n; h.nz = 1 / n; }
				}
				if(fabsf(dz) < 0.01f && dx > 0.5f && parapet)						// raio horizontal: parapeito 20 alem da parede
				{
					const float start = x, wallAt = 20.0f;
					if(start < wallAt && wallAt - start <= maxDist) { h.hit = true; h.dist = wallAt - start; h.x = sx + h.dist; h.z = sz; h.nx = -1; }
				}
				return h;
			}
		};
		float fw = 0, sz = 0;
		{
			SlopeWorld w = { 500.0f, 0.0f, false };
			bool ok = FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f, &fw, &sz);
			CHECK(ok && fabsf(fw - 70.0f) < 0.01f && fabsf(sz - 500.0f) < 0.01f, "chao plano: fica a 40 alem da parede, a altura do topo");
		}
		{
			SlopeWorld w = { 500.0f, 1.0f, false };								// 45 graus (normal.z 0,71)
			bool ok = FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f + 25.0f, &fw, &sz);	// topo medido a wall+25 (x = 25): z = 525
			// no ponto de chegada (x = 40) o chao esta a 540: 15 acima do que o codigo antigo usava (era o enterramento)
			CHECK(ok && fabsf(fw - 70.0f) < 0.01f && fabsf(sz - 540.0f) < 0.01f, "rampa de 45 graus: a altura final e medida no ponto de chegada (540, nao os 525 do topo medido)");
		}
		{
			SlopeWorld w = { 500.0f, 2.0f, false };								// 63 graus (normal.z 0,45): nao andavel
			CHECK(!FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 530.0f, &fw, &sz), "rampa ingreme (normal.z 0,45): nao ha onde ficar de pe");
		}
		{
			SlopeWorld w = { 500.0f, 0.0f, true };								// parapeito a 20 alem da parede: bloqueia o caminho
			CHECK(!FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f, &fw, &sz), "parapeito pelo caminho: recusa");
		}
		{
			World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 500});			// parede com topo plano em z=500, 200 de largura
			bool ok = FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f, &fw, &sz);
			CHECK(ok && fabsf(sz - 500.0f) < 0.01f, "topo plano (caixa): aceita");
		}
		{
			World w; w.boxes.push_back({0, 28, -BIG, BIG, -10, 500});			// borda estreita: so 28 de largura alem do plano
			bool ok = FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f, &fw, &sz);
			CHECK(ok && fabsf(fw - 54.0f) < 0.01f, "borda estreita (28): recua para o ponto a 24 da parede, em vez de ficar no ar");
		}
		{
			World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 500});
			w.boxes.push_back({0, 200, -BIG, BIG, 560, 600});					// teto baixo: sem espaco para a cabeca
			CHECK(!FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f, &fw, &sz), "teto baixo: recusa");
		}
		{
			World w; w.boxes.push_back({0, 5, -BIG, BIG, -10, 500});			// topo de 5 de largura: nao ha onde ficar
			CHECK(!FindStandingSpot(w, 1.0f, 0.0f, -30.0f, 0.0f, 30.0f, 500.0f, &fw, &sz), "topo de 5 de largura: nao ha onde ficar de pe");
		}
	}

	printf("ENTRADAS INVALIDAS\n");
	{
		World w; w.boxes.push_back({0, 200, -BIG, BIG, -10, 600});
		const char * why = "";
		float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
		CHECK(!ProbeEntryWall(w, nan, fy, px, py, 0.0f, 80.0f, 65.0f, &why).found, "direcao NaN: sem parede");
		CHECK(!ProbeEntryWall(w, fx, fy, inf, py, 0.0f, 80.0f, 65.0f, &why).found, "posicao infinita: sem parede");
		CHECK(!PlanOverhangHop(w, fx, fy, nan, py, pz, 125.0f, 180.0f, 30.0f).ok, "beiral com NaN: recusa");
		CHECK(ClearancePush(w, fx, fy, px, py, nan, 30.0f) == 0.0f, "afastamento com NaN: 0");
	}

	printf("\n%s (%d falhas)\n", g_fail ? "FALHOU" : "TUDO OK", g_fail);
	return g_fail ? 1 : 0;
}
