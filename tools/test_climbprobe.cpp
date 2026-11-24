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
		ok = ok && (p - afterOne) < 1.1f;						// desce no maximo ~1 unidade por quadro
		CHECK(ok, "afastamento: sobe depressa (25 em < 0,5 s), desce devagar (60/s)");
		CHECK(StepPush(10.0f, 10.0f, -1.0f) == 10.0f && StepPush(10.0f, 10.0f, 0.0f) == 10.0f, "dt invalido: nao muda");
		CHECK(!HoldPosition(false, 0.0f, 0.0f, 1.0f), "em movimento nunca fica parado");
		CHECK(!HoldPosition(true, 4.0f, 0.0f, 1.0f), "parado mas o erro passa da zona morta: corrige");
		CHECK(!HoldPosition(true, 0.0f, 0.0f, 0.9f), "parado mas a parede girou: corrige");
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
