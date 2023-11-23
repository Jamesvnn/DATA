#include "udf.h"
/*
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)>(b))?(b):(a))
*/
DEFINE_INIT(patching, d)
{
	cell_t c;
	Thread* t;
	Thread* ts;
	Thread* tp;

	real xc[ND_ND], dist2;
#if !RP_HOST
		/* loop over all cell threads in the domain */
	thread_loop_c(t, d)
	{
		ts = THREAD_SUB_THREAD(t, 1);
		tp = THREAD_SUB_THREAD(t, 0);
		/* loop over all cells */
		begin_c_loop(c, t)
		{
			C_CENTROID(xc, c, t);
			dist2 = xc[0] * xc[0] + (xc[1] - 0.0015) * (xc[1] - 0.0015) + xc[2] * xc[2];

			if (dist2 < 0.0015 * 0.0015)
			{
				C_VOF(c, ts) = 1.000000;
				C_VOF(c, tp) = 0.000000;
				C_V(c, t)    = 0.0;
			}
			else
			{
				C_VOF(c, ts) = 0.000000;
				C_VOF(c, tp) = 1.000000;
			}
			
		}
		end_c_loop(c, t)
	}
	Message("Patched successfully using UDF!\n");
#endif
}

DEFINE_EXECUTE_AT_END(execute_at_end)
{
	Domain* d;
	Thread* t;
	Thread* tp;

	Thread* tf;

	cell_t c;
	face_t f;
	d = Get_Domain(1);  /* mixture domain if multiphase */
	real Height, Radius, Depth, Speed, xc[ND_ND], xf[ND_ND];
	/* loop over all cell threads in the domain */
	Height = -10.;
	Radius = 0.;
	Depth  = 0.;
	Speed = 1e+20;

#if !RP_HOST
	/* height */
	
	thread_loop_c(t, d)
	{
		
		tp = THREAD_SUB_THREAD(t, 0);
		
		// loop over all cells
		begin_c_loop(c, t)
		{
			C_CENTROID(xc, c, t);
			
			if (C_VOF(c, tp) < 0.5)
			{
				Height = MAX(Height, xc[1]);
				Depth  = MIN(Depth,  xc[1]);

				if (Depth == xc[1])
					Speed = C_V(c, t);
			}
		}
		end_c_loop(c, t)
	}
	Height = PRF_GRHIGH1(Height);
	Depth  = PRF_GRLOW1 (Depth);
	
	/* contact line*/
	tf = Lookup_Thread(d, 9); // 9 is wall ID of bottom wall
	tp = THREAD_SUB_THREAD(THREAD_T0(tf), 0);

	begin_f_loop(f, tf)
	{
		if PRINCIPAL_FACE_P(f, tf)
		{
			F_CENTROID(xf, f, tf);
			if (C_VOF(F_C0(f, tf), tp) < 0.5)
			{
				Radius = MAX(Radius, xf[0]);
			}
			else
			{
			}
		}
	}
	end_f_loop(f, tf)
	Radius = PRF_GRHIGH1(Radius);
#endif

	node_to_host_real(&Radius, 1);
	node_to_host_real(&Height, 1);
	node_to_host_real(&Depth, 1);
	node_to_host_real(&Speed, 1);

#if !RP_NODE
	FILE* fp = NULL;

	if ((fp = fopen("contact.txt", "a")) != NULL)
	{
		fprintf(fp, "%f,\t%f,\t%f,\t%f,\t%f\n", CURRENT_TIME, Height, Radius, fabs(Depth), fabs(Speed));
	}
	fclose(fp);
#endif
}

