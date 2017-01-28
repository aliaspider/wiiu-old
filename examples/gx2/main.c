#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <wiiu/vpad.h>
#include <sys/socket.h>
#include <wiiu/os.h>
#include <wiiu/procui.h>
#include <wiiu/gx2.h>
#include <system/memory.h>
#include "shader/tex_shader.h"
#include "system/exception_handler.h"
#include "system/logger.h"
#include "system/wiiu_dbg.h"

typedef struct
{
   int width;
   int height;
   GX2TVRenderMode mode;
} render_mode_t;

void* drc_scan_buffer;
void* tv_scan_buffer;
GX2ColorBuffer color_buffer;
GX2ContextState* ctx_state;
void* cmd_buffer;

static const render_mode_t render_mode_map[] =
{
   {0},                                         /* GX2_TV_SCAN_MODE_NONE  */
   {854,  480,  GX2_TV_RENDER_MODE_WIDE_480P},  /* GX2_TV_SCAN_MODE_576I  */
   {854,  480,  GX2_TV_RENDER_MODE_WIDE_480P},  /* GX2_TV_SCAN_MODE_480I  */
   {854,  480,  GX2_TV_RENDER_MODE_WIDE_480P},  /* GX2_TV_SCAN_MODE_480P  */
   {1280, 720,  GX2_TV_RENDER_MODE_WIDE_720P},  /* GX2_TV_SCAN_MODE_720P  */
   {0},                                         /* GX2_TV_SCAN_MODE_unk   */
   {1920, 1080, GX2_TV_RENDER_MODE_WIDE_1080P}, /* GX2_TV_SCAN_MODE_1080I */
   {1920, 1080, GX2_TV_RENDER_MODE_WIDE_1080P}  /* GX2_TV_SCAN_MODE_1080P */
};

uint32_t* create_mandelbrot(int width, int height)
{
   int i, x, y;

   uint16_t max = 0x20;
   float x0 = 0.36;
   float x1 = 0.60;
   float y0 = 0.36;
   float y1 = 0.80;

   int full_w = width / (x1 - x0);
   int full_h = height / (y1 - y0);

   int off_x = width * x0;
   int off_y = height * y0;

   uint32_t* data = (uint32_t*)malloc(width * height * sizeof(uint32_t));
   uint16_t* data_cnt = (uint16_t*)malloc(full_w * full_h * sizeof(uint16_t));
   uint32_t palette [max];
   memset(palette, 0, sizeof(palette));

   for (i = 0; i < full_w * full_h; i++)
   {
      float real  = ((i % full_w) - (full_w / 2.0)) * (4.0 / full_w);
      float im    = ((i / full_w) - (full_h / 2.0)) * (4.0 / full_w);
      float xx    = 0;
      float yy    = 0;
      int counter = 0;

      while (xx * xx + yy * yy <= 4 && counter < max)
      {
         float tmp = xx * xx - yy * yy + real;
         yy = 2 * xx * yy + im;
         xx = tmp;
         counter++;
      }

      data_cnt[i] = counter;
   }

   for (i = 0; i < full_w * full_h; i++)
      palette[data_cnt[i] - 1]++;

   for (i = 1; i < max; i++)
      palette[i] += palette[i - 1];

   for (y = 0; y < height; y++)
      for (x = 0; x < width; x++)
         data[x + y * width] = max * palette[data_cnt[x + off_x + (y + off_y) * full_w] - 1] | 0xff;


   free(data_cnt);
   return data;
}

void SaveCallback()
{
   OSSavesDone_ReadyToRelease();
}

int main(int argc, char **argv)
{
   setup_os_exceptions();
   ProcUIInit(&SaveCallback);
   socket_lib_init();
#if defined(LOGGER_IP) && defined(LOGGER_TCP_PORT)
   log_init(LOGGER_IP, LOGGER_TCP_PORT);
#endif
   VPADInit();

   int i;

   VPADInit();

   /* video init */
   cmd_buffer = MEM2_alloc(0x400000, 0x40);
   u32 init_attributes[] =
   {
      GX2_INIT_CMD_BUF_BASE, (u32)cmd_buffer,
      GX2_INIT_CMD_BUF_POOL_SIZE, 0x400000,
      GX2_INIT_ARGC, 0,
      GX2_INIT_ARGV, 0,
      GX2_INIT_END
   };
   GX2Init(init_attributes);

   /* setup scanbuffers */
   u32 size = 0;
   u32 tmp = 0;
   const render_mode_t* render_mode = &render_mode_map[GX2GetSystemTVScanMode()];
   GX2CalcTVSize(render_mode->mode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &size,&tmp);
   tv_scan_buffer = MEMBucket_alloc(size, GX2_SCAN_BUFFER_ALIGNMENT);
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU, tv_scan_buffer, size);
   GX2SetTVBuffer(tv_scan_buffer, size, render_mode->mode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);

   GX2CalcDRCSize(GX2_DRC_RENDER_MODE_SINGLE, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &size, &tmp);
   drc_scan_buffer = MEMBucket_alloc(size, GX2_SCAN_BUFFER_ALIGNMENT);
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU, drc_scan_buffer, size);
   GX2SetDRCBuffer(drc_scan_buffer, size, GX2_DRC_RENDER_MODE_SINGLE, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);

   memset(&color_buffer, 0, sizeof(GX2ColorBuffer));
   color_buffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
   color_buffer.surface.width = render_mode->width;
   color_buffer.surface.height = render_mode->height;
   color_buffer.surface.depth = 1;
   color_buffer.surface.mipLevels = 1;
   color_buffer.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
   color_buffer.surface.use = GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV;
   color_buffer.viewNumSlices = 1;
   GX2CalcSurfaceSizeAndAlignment(&color_buffer.surface);
   GX2InitColorBufferRegs(&color_buffer);

   color_buffer.surface.image = MEM1_alloc(color_buffer.surface.imageSize, color_buffer.surface.alignment);
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU, color_buffer.surface.image, color_buffer.surface.imageSize);

   ctx_state = (GX2ContextState*)MEM2_alloc(sizeof(GX2ContextState), GX2_CONTEXT_STATE_ALIGNMENT);
   GX2SetupContextStateEx(ctx_state, GX2_TRUE);

   GX2SetContextState(ctx_state);
   GX2SetColorBuffer(&color_buffer, GX2_RENDER_TARGET_0);
   GX2SetViewport(0.0f, 0.0f, color_buffer.surface.width, color_buffer.surface.height, 0.0f, 1.0f);
   GX2SetScissor(0, 0, color_buffer.surface.width, color_buffer.surface.height);
   GX2SetDepthOnlyControl(GX2_DISABLE, GX2_DISABLE, GX2_COMPARE_FUNC_ALWAYS);
   GX2SetColorControl(GX2_LOGIC_OP_COPY, 1, GX2_DISABLE, GX2_ENABLE);
   GX2SetBlendControl(GX2_RENDER_TARGET_0, GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
                      GX2_ENABLE,          GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
   GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, GX2_DISABLE, GX2_DISABLE);
   GX2SetSwapInterval(1);

   /* GX2 can't access memory < 0x10000000 so we have to copy tex_shader to the heap
    * for it to be compatible with elf builds. this isn't necessary for RPX builds
    * since the .data section starts at 0x10000000 */

   tex_shader_t* shader = MEM2_alloc(sizeof(tex_shader), 0x1000);
   memcpy(shader, &tex_shader, sizeof(tex_shader));
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU, shader, sizeof(tex_shader));

   shader->vs.program = MEM2_alloc(shader->vs.size, GX2_SHADER_ALIGNMENT);
   memcpy(shader->vs.program, tex_shader.vs.program, shader->vs.size);
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, shader->vs.program, shader->vs.size);
   shader->vs.attribVars = MEM2_alloc(shader->vs.attribVarCount * sizeof(GX2AttribVar),
         GX2_SHADER_ALIGNMENT);
   memcpy(shader->vs.attribVars, tex_shader.vs.attribVars ,
          shader->vs.attribVarCount * sizeof(GX2AttribVar));

   shader->ps.program = MEM2_alloc(shader->ps.size, GX2_SHADER_ALIGNMENT);
   memcpy(shader->ps.program, tex_shader.ps.program, shader->ps.size);
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, shader->ps.program, shader->ps.size);
   shader->ps.samplerVars = MEM2_alloc(shader->ps.samplerVarCount * sizeof(GX2SamplerVar),
         GX2_SHADER_ALIGNMENT);
   memcpy(shader->ps.samplerVars, tex_shader.ps.samplerVars,
          shader->ps.samplerVarCount * sizeof(GX2SamplerVar));

   /* init shader */

   shader->fs.size = GX2CalcFetchShaderSizeEx(2, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
   shader->fs.program = MEM2_alloc(shader->fs.size, GX2_SHADER_ALIGNMENT);

   GX2InitFetchShaderEx(&shader->fs, (uint8_t*)shader->fs.program, sizeof(shader->attribute_stream) / sizeof(GX2AttribStream),
                        (GX2AttribStream*)&shader->attribute_stream,
                        GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, shader->fs.program, shader->fs.size);

   GX2SetVertexShader(&shader->vs);
   GX2SetPixelShader(&shader->ps);
   GX2SetFetchShader(&shader->fs);

   /* set up Attribute Buffers */
   struct
   {
      float x, y;
   }* position_vb = MEM2_alloc(4 * sizeof(*position_vb), GX2_VERTEX_BUFFER_ALIGNMENT);

   position_vb[0].x = -1.0f;
   position_vb[0].y = -1.0f;
   position_vb[1].x =  1.0f;
   position_vb[1].y = -1.0f;
   position_vb[2].x =  1.0f;
   position_vb[2].y =  1.0f;
   position_vb[3].x = -1.0f;
   position_vb[3].y =  1.0f;

   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, position_vb, 4 * sizeof(*position_vb));

   struct
   {
      float u, v;
   }* tex_coord_vb = MEM2_alloc(4 * sizeof(*tex_coord_vb), GX2_VERTEX_BUFFER_ALIGNMENT);

   tex_coord_vb[0].u = 0.0f;
   tex_coord_vb[0].v = 1.0f;
   tex_coord_vb[1].u = 1.0f;
   tex_coord_vb[1].v = 1.0f;
   tex_coord_vb[2].u = 1.0f;
   tex_coord_vb[2].v = 0.0f;
   tex_coord_vb[3].u = 0.0f;
   tex_coord_vb[3].v = 0.0f;

   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, tex_coord_vb, 4 * sizeof(*tex_coord_vb));

   GX2SetAttribBuffer(0, 4 * sizeof(*position_vb), sizeof(*position_vb), position_vb);
   GX2SetAttribBuffer(1, 4 * sizeof(*tex_coord_vb), sizeof(*tex_coord_vb), tex_coord_vb);

   /* init an empty texture */
   int width = 800;
   int height = 480;
   GX2Texture texture;
   memset(&texture, 0, sizeof(GX2Texture));
   texture.surface.width    = width;
   texture.surface.height   = height;
   texture.surface.depth    = 1;
   texture.surface.dim      = GX2_SURFACE_DIM_TEXTURE_2D;
   texture.surface.format   = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
   texture.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
   texture.viewNumSlices    = 1;
   texture.compMap          = GX2_COMP_SEL(_X_, _Y_, _Z_, _W_);
   GX2CalcSurfaceSizeAndAlignment(&texture.surface);
   GX2InitTextureRegs(&texture);

   texture.surface.image = MEM2_alloc(texture.surface.imageSize,
                                       texture.surface.alignment);
   /* create an image so we have something to display */   
   uint32_t* data = create_mandelbrot(width, height);
   uint32_t* dst = (uint32_t*)texture.surface.image;
   uint32_t* src = (uint32_t*)data;

   for (i = 0; i < texture.surface.height; i++)
   {
      memcpy(dst, src, width * sizeof(uint32_t));
      dst += texture.surface.pitch;
      src += width;
   }
   free(data);

   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, texture.surface.image,
                 texture.surface.imageSize);
   /* create a sampler */
   GX2Sampler sampler;
   GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);

   /* set Texture and Sampler */
   GX2SetPixelTexture(&texture, tex_shader.sampler.location);
   GX2SetPixelSampler(&sampler, tex_shader.sampler.location);

   int frames = 0;
   GX2SetTVEnable(GX2_ENABLE);
   GX2SetDRCEnable(GX2_ENABLE);
   while (true)
   {
      VPADStatus vpad;
      VPADReadError vpad_error;
      VPADRead(0, &vpad, 1, &vpad_error);

      if (vpad.trigger & VPAD_BUTTON_HOME || vpad.trigger & VPAD_BUTTON_B)
         break;

      /* can't call GX2ClearColor after GX2SetContextState for whatever reason */
      GX2ClearColor(&color_buffer, 0.0f, 0.0f, 0.0f, 1.0f);

      GX2SetContextState(ctx_state);

      GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, 4, 0, 1);

      GX2CopyColorBufferToScanBuffer(&color_buffer, GX2_SCAN_TARGET_DRC);
      GX2CopyColorBufferToScanBuffer(&color_buffer, GX2_SCAN_TARGET_TV);

      GX2SwapScanBuffers();
      GX2Flush();
      GX2WaitForVsync();

      printf("\rframe : %5i", frames++);
      fflush(stdout);
   };

   GX2Flush();
   GX2DrawDone();
   GX2Shutdown();

   MEM2_free(shader->vs.program);
   MEM2_free(shader->vs.attribVars);
   MEM2_free(shader->ps.program);
   MEM2_free(shader->ps.samplerVars);
   MEM2_free(shader->fs.program);
   MEM2_free(shader);


   MEM2_free(position_vb);
   MEM2_free(tex_coord_vb);

   MEM2_free(cmd_buffer);
   MEMBucket_free(tv_scan_buffer);
   MEMBucket_free(drc_scan_buffer);
   MEM1_free(color_buffer.surface.image);
   MEM2_free(ctx_state);

   MEM2_free(texture.surface.image);

   ProcUIShutdown();

#if defined(LOGGER_IP) && defined(LOGGER_TCP_PORT)
   log_deinit();
#endif
   return 0;
}
