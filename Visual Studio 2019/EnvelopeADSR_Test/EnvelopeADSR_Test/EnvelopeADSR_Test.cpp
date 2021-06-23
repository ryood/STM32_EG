/*
 * ADSR Envelope Test
 *
 * 2021.06.22
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define ATTACK_THRESHOLD  (0.6f)

typedef enum {
	stNone,
	stAttack,
	stDecay,
	stRelease
} EGState;

typedef struct {
	float attackR;			// kΩ
	float decayR;			// kΩ
	float sustainLevel;		// 0.0f ~ ATTACK_THTESHOLD
	float releaseR;			// kΩ
	float capacitance;		// uF

	EGState state;
	float samplingPeriod;	// second
	uint32_t tick;
	float amplitude;
	float v1;
	float v2;
	float v3;
	float t1;
	float t2;
} Envelope;

void Envelope_init(Envelope* p_env, float samplingPeriod);
float Envelope_step(Envelope* p_env);
void Envelope_gateOn(Envelope* p_env);
void Envelope_gateOff(Envelope* p_env);

void Envelope_init(Envelope* p_env, float samplingPeriod)
{
	p_env->samplingPeriod = samplingPeriod;
	p_env->state = stNone;
	p_env->amplitude = 0.0f;
	p_env->v3 = 0.0f;
}

void Envelope_gateOn(Envelope* p_env)
{
	p_env->v3 = p_env->amplitude;
	p_env->tick = 0;
	p_env->state = stAttack;
}

void Envelope_gateOff(Envelope* p_env)
{
	p_env->v2 = p_env->amplitude;
	p_env->t2 = p_env->tick * p_env->samplingPeriod;
	p_env->state = stRelease;
}

static float attack(Envelope* p_env, float t)
{
	float tau = p_env->attackR * p_env->capacitance / 1000.0f;
	return 1.0f - ( 1.0f - p_env->v3 ) * expf( -t / tau );
}

static float decay(Envelope* p_env, float t)
{
	float tau = p_env->decayR * p_env->capacitance / 1000.0f;
	return ( p_env->v1 - p_env->sustainLevel ) * expf( -( t - p_env->t1 ) / tau ) + p_env->sustainLevel;
}

static float release(Envelope* p_env, float t)
{
	float tau = p_env->releaseR * p_env->capacitance / 1000.0f;
	return p_env->v2 * expf( -( t - p_env->t2 ) / tau );
}

float Envelope_step(Envelope* p_env)
{
	float t = p_env->tick * p_env->samplingPeriod;

	switch (p_env->state) {
	case stNone:
		break;
	case stAttack:
		p_env->amplitude = attack(p_env, t);
		if (p_env->amplitude > ATTACK_THRESHOLD) {
			p_env->v1 = p_env->amplitude;
			p_env->t1 = t;
			p_env->state = stDecay;
		}
		break;
	case stDecay:
		p_env->amplitude = decay(p_env, t);
		break;
	case stRelease:
		p_env->amplitude = release(p_env, t);
		break;
	}

	p_env->tick++;

	if (p_env->tick == INT32_MAX) {
		perror("tick overflow\n");
	}

	return p_env->amplitude;
}

#define LOOP_N (2)
#define STEP_LENGTH (1000)
#define GATE_LENGTH (500)
#define SAMPLING_PERIOD (0.001f)

int main()
{
	Envelope env;

	Envelope_init(&env, SAMPLING_PERIOD);
	env.capacitance = 22;
	env.attackR = 3;
	env.decayR = 4;
	env.releaseR = 10;
	env.sustainLevel = 0.3;

	for (int j = 0; j < LOOP_N; j++) {
		Envelope_gateOn(&env);
		for (int i = 0; i < STEP_LENGTH; i++) {
			if (i == GATE_LENGTH) {
				Envelope_gateOff(&env);
			}
			float v = Envelope_step(&env);
			printf("%f\n", v);
		}
	}
}
