// FreeClimb LE - wall climbing for Skyrim Legendary Edition.
// Copyright (c) 2026 Ezyel. Port of FreeClimb by Epsilona (Copyright (c) 2026 Epsilona), used with the author's permission.
// SPDX-License-Identifier: GPL-3.0-only
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
// Public License, version 3, as published by the Free Software Foundation. It is distributed WITHOUT ANY
// WARRANTY; see the LICENSE file.
//
// ClimbProbe.h - a geometria de decisao que NAO depende do jogo: so recebe uma funcao "raio" e devolve respostas.
//
//   * ProbeEntryWall      - a parede a frente e mesmo uma parede (e nao um bau, uma pedra pequena, um degrau)?
//   * ClearancePush       - quanto afastar o corpo da parede para nao entrar numa saliencia a outra altura.
//   * ProbeRecessedWall   - a parede recuou (a saliencia acabou): continua mais atras?
//   * PlanOverhangHop     - ha um beiral/cornija por cima da cabeca: da para saltar por cima dele?
//
// Esta separacao existe para se poder testar tudo contra um mundo simulado (tools/test_climbprobe.cpp), sem o Skyrim.
// 'Ray' e qualquer objeto chamavel:  Hit ray(sx, sy, sz, dx, dy, dz, maxDist)   (direcao unitaria; z para cima).
#pragma once

#include <math.h>

namespace ClimbProbe
{
	struct Hit  { bool hit; float dist, x, y, z, nx, ny, nz; };
	struct Wall { bool found; float dist, x, y, z, nx, ny; };

	// Os mesmos limiares do ProbeWall do Gameplay.cpp.
	const float kMaxWallNormalZ = 0.45f;		// |normal.z| acima disto: nao e uma parede
	const float kFacingDot		= -0.3f;		// normal . direcao do raio: tem de estar virada para nos

	// Um numero que pode ir para o Havok: nem NaN, nem infinito, nem absurdamente grande.
	// (Um NaN entregue ao Havok e uma das maneiras classicas de o jogo bloquear sem mensagem nenhuma.)
	inline bool Finite(float v) { return v == v && v > -1.0e7f && v < 1.0e7f; }

	inline Wall NoWall() { Wall w; w.found = false; w.dist = w.x = w.y = w.z = w.nx = w.ny = 0.0f; return w; }

	inline bool WallLike(const Hit & h, float fx, float fy)
	{
		if(!h.hit) return false;
		if(!Finite(h.nx) || !Finite(h.ny) || !Finite(h.nz) || !Finite(h.dist)) return false;
		if(fabsf(h.nz) > kMaxWallNormalZ) return false;
		if(h.nx * fx + h.ny * fy > kFacingDot) return false;
		return true;
	}

	// Um raio horizontal na direcao (fx,fy) a altura z: devolve a parede que acerta, se for uma parede.
	template<class Ray>
	Wall SampleWall(Ray & ray, float fx, float fy, float px, float py, float z, float maxDist)
	{
		Wall w = NoWall();
		if(!Finite(fx) || !Finite(fy) || !Finite(px) || !Finite(py) || !Finite(z) || !(maxDist > 0.0f)) return w;

		const Hit h = ray(px, py, z, fx, fy, 0.0f, maxDist);
		if(!WallLike(h, fx, fy)) return w;

		const float len = sqrtf(h.nx * h.nx + h.ny * h.ny);
		if(!(len > 0.01f)) return w;
		w.found = true; w.dist = h.dist; w.x = h.x; w.y = h.y; w.z = h.z;
		w.nx = h.nx / len; w.ny = h.ny / len;
		return w;
	}

	// ------------------------------------------------------------------------------------------------------------
	// ENTRADA: uma parede de verdade tem de existir ACIMA da altura minima, e em dois niveis seguidos.
	//
	// Antes, a entrada usava tres raios a 55, 95 e 135 acima dos pes e agarrava-se ao primeiro que acertasse em
	// qualquer coisa. O MinLedge so era lido ao passar por cima da borda, nao ao agarrar: um bau, uma pedra pequena,
	// uma mesa, um degrau, tudo servia. Agora os dois raios ficam acima da altura minima (relativa ao CHAO, para o
	// caso de se estar no ar), e os dois tem de acertar no mesmo plano.
	//
	// 'baseZ' = a altura do chao junto ao jogador. 'minHeight' = altura minima do obstaculo.
	template<class Ray>
	Wall ProbeEntryWall(Ray & ray, float fx, float fy, float px, float py, float baseZ, float minHeight, float maxDist, const char ** reason)
	{
		const float z1 = baseZ + minHeight + 6.0f, z2 = z1 + 24.0f;

		const Wall a = SampleWall(ray, fx, fy, px, py, z1, maxDist);
		if(!a.found) { *reason = "nothing tall enough ahead (low obstacle, left to the normal jump / parkour mod)"; return NoWall(); }

		const Wall b = SampleWall(ray, fx, fy, px, py, z2, maxDist);
		if(!b.found || fabsf(b.dist - a.dist) > 30.0f) { *reason = "the obstacle is too short to be a wall"; return NoWall(); }

		return a.dist <= b.dist ? a : b;
	}

	// ------------------------------------------------------------------------------------------------------------
	// CORPO LIVRE: o plano da parede e lido a altura do peito. Se a outra altura (pes, cabeca) houver uma saliencia
	// mais perto, o corpo entra nela. Devolve quanto afastar o personagem (0 a maxPush) para nao a atravessar.
	//
	// (fx,fy) = a direcao do raio original; 'chestDist' = a distancia medida a altura do peito, a partir de (px,py).
	template<class Ray>
	float ClearancePush(Ray & ray, float fx, float fy, float px, float py, float pz, float chestDist, float comfort = 14.0f, float maxPush = 40.0f)
	{
		if(!Finite(fx) || !Finite(fy) || !Finite(px) || !Finite(py) || !Finite(pz) || !(chestDist > 0.0f)) return 0.0f;

		static const float heights[2] = { 20.0f, 115.0f };
		float worst = 0.0f;
		for(int k = 0; k < 2; ++k)
		{
			const Hit h = ray(px, py, pz + heights[k], fx, fy, 0.0f, chestDist + 10.0f);
			if(!WallLike(h, fx, fy)) continue;
			const float d = chestDist - h.dist;			// > 0: nesta altura a superficie esta mais perto do que a do peito
			if(d > worst) worst = d;
		}

		float push = worst - comfort;
		if(push < 0.0f) push = 0.0f;
		if(push > maxPush) push = maxPush;
		return push;
	}

	// ------------------------------------------------------------------------------------------------------------
	// PAREDE RECUADA: a escalar na vertical, o raio ao peito deixou de acertar (a saliencia acabou e a parede recuou).
	// Procura mais atras, ate 'extra' unidades alem do alcance normal, e so aceita a MESMA parede (normal parecida).
	template<class Ray>
	Wall ProbeRecessedWall(Ray & ray, float nx0, float ny0, float px, float py, float pz, float gap, float extra)
	{
		const Wall w = SampleWall(ray, -nx0, -ny0, px, py, pz + 70.0f, gap + extra);
		if(!w.found) return w;
		if(w.nx * nx0 + w.ny * ny0 < 0.8f) return NoWall();		// outra parede (canto, outro edificio): nao e esta
		return w;
	}

	// ------------------------------------------------------------------------------------------------------------
	// BEIRAL POR CIMA DA CABECA: a escalar, o personagem chega a face de baixo de uma saliencia (beiral, cornija,
	// varanda) e nao passa. Planeia um salto para CIMA dela, rente a parede, que e onde a parede continua.
	//
	// Condicoes (cada uma recusa com uma razao, para o log):
	//   1. ha mesmo algo por cima da cabeca;
	//   2. a saliencia tem um topo onde se pode ficar (normal.z >= 0.5) a uma subida razoavel (<= maxRise);
	//   3. ha espaco por cima desse topo;
	//   4. e um BEIRAL, nao um piso ou um teto: o seu bordo exterior esta a menos de kEdgeProbe do personagem.
	//      (Se nao se encontra o bordo, a saliencia e larga demais e nao se salta por ela.)
	struct Overhang { bool ok; float underZ, topZ, destZ, edgeOut; bool wallAbove; const char * reason; };

	template<class Ray>
	Overhang PlanOverhangHop(Ray & ray, float fx, float fy, float px, float py, float pz, float headHeight, float maxRise, float gap)
	{
		Overhang o; o.ok = false; o.underZ = o.topZ = o.destZ = o.edgeOut = 0.0f; o.wallAbove = false; o.reason = "";
		if(!Finite(fx) || !Finite(fy) || !Finite(px) || !Finite(py) || !Finite(pz)) { o.reason = "invalid position"; return o; }

		const float kEdgeProbe = 100.0f, kMaxEdgeOut = 80.0f;

		// 1. a face de baixo
		const Hit up = ray(px, py, pz + headHeight, 0.0f, 0.0f, 1.0f, 110.0f);
		if(!up.hit || !Finite(up.z)) { o.reason = "nothing above the head"; return o; }
		o.underZ = up.z;

		// 2. o topo, a partir de cima (comeca 100 acima da face de baixo)
		const Hit down = ray(px, py, o.underZ + 100.0f, 0.0f, 0.0f, -1.0f, 100.0f);
		if(!down.hit || !Finite(down.z) || down.nz < 0.5f) { o.reason = "no walkable top on the obstruction (too thick, or a steep roof)"; return o; }
		o.topZ = down.z;
		o.destZ = o.topZ + 4.0f;
		if(o.destZ - pz > maxRise) { o.reason = "obstruction too high to get over"; return o; }
		if(o.topZ < pz + headHeight - 5.0f) { o.reason = "inconsistent obstruction"; return o; }

		// 3. espaco por cima do topo
		const Hit roof = ray(px, py, o.topZ + 8.0f, 0.0f, 0.0f, 1.0f, 110.0f);
		if(roof.hit && roof.dist < 100.0f) { o.reason = "no head room above the obstruction"; return o; }

		// 4. e um beiral? Raio de FORA para a parede, a meio da espessura: tem de bater na face exterior da saliencia.
		const float zMid = 0.5f * (o.underZ + o.topZ);
		const Hit edge = ray(px - fx * kEdgeProbe, py - fy * kEdgeProbe, zMid, fx, fy, 0.0f, kEdgeProbe);
		if(!edge.hit) { o.reason = "obstruction too wide (a floor or ceiling, not an eave)"; return o; }
		if(edge.nx * fx + edge.ny * fy > -0.2f) { o.reason = "obstruction edge does not face out"; return o; }
		o.edgeOut = kEdgeProbe - edge.dist;
		if(o.edgeOut > kMaxEdgeOut) { o.reason = "obstruction sticks out too far"; return o; }

		// informativo: a parede continua por cima do topo?
		const Wall above = SampleWall(ray, fx, fy, px, py, o.destZ + 70.0f, gap + 50.0f);
		o.wallAbove = above.found;

		o.ok = true;
		return o;
	}
}
