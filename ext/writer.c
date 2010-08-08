#include "ruby_vips.h"
#include "image.h"

static VALUE cVIPSWriter;

static VALUE
writer_initialize(VALUE obj, VALUE image)
{
    GetImg(image, data, im);
    GetImg(obj, data_new, im_new);

    img_add_dep(data_new, image);
    if (im_copy(im, im_new))
        vips_lib_error();

    return obj;
}

static VALUE
writer_image(VALUE obj)
{
    GetImg(obj, data, im);

    if(data->deps)
        return data->deps[0];

    return Qnil;
}

static VALUE
writer_meta_get(VALUE obj, const char* name)
{
    GetImg(obj, data, im);

    void *buf;
	size_t len;

	if (im_meta_get_blob(im, name, &buf, &len))
		return Qnil;

	return rb_tainted_str_new((char *)buf, len);
}

static VALUE
writer_meta_p(VALUE obj, const char* name)
{
    GetImg(obj, data, im);

	if (im_header_get_typeof(im, name))
		return Qtrue;

    return Qfalse;
}

static VALUE
writer_meta_set(VALUE obj, const char* name, VALUE str)
{
    GetImg(obj, data, im);

	size_t len = RSTRING_LEN(str);
    void *buf = malloc(len);
    memcpy(buf, RSTRING_PTR(str), len);

    if (im_meta_set_blob(im, name, (im_callback_fn)xfree, buf, len)) {
        xfree(buf);
        vips_lib_error();
    }

    return str;
}

static VALUE
writer_meta_remove(VALUE obj, const char* name)
{
    GetImg(obj, data, im);

	if (im_meta_remove(im, name))
		return Qfalse;

    return Qtrue;
}

static VALUE
writer_exif(VALUE obj)
{
    return writer_meta_get(obj, IM_META_EXIF_NAME);
}

static VALUE
writer_exif_p(VALUE obj)
{
    return writer_meta_p(obj, IM_META_EXIF_NAME);
}

static VALUE
writer_exif_set(VALUE obj, VALUE str)
{
    return writer_meta_set(obj, IM_META_EXIF_NAME, str);
}

static VALUE
writer_remove_exif(VALUE obj)
{
    return writer_meta_remove(obj, IM_META_EXIF_NAME);
}

static VALUE
writer_icc(VALUE obj)
{
    return writer_meta_get(obj, IM_META_ICC_NAME);
}

static VALUE
writer_icc_p(VALUE obj)
{
    return writer_meta_p(obj, IM_META_ICC_NAME);
}

static VALUE
writer_icc_set(VALUE obj, VALUE str)
{
    return writer_meta_set(obj, IM_META_ICC_NAME, str);
}

static VALUE
writer_remove_icc(VALUE obj)
{
    return writer_meta_remove(obj, IM_META_ICC_NAME);
}

static VALUE
writer_jpeg_buf(VALUE obj, VALUE quality)
{
    VipsImage *im_out;
    char *buf = NULL;
    int length;
    GetImg(obj, data, im);

    if (!(im_out = im_open("writer_jpeg_buf", "p")))
        vips_lib_error();

    if (im_vips2bufjpeg(im, im_out, NUM2INT(quality), &buf, &length)) {
		im_close(im_out);
        vips_lib_error();
	}

    im_close(im_out);

    return rb_tainted_str_new(buf, length);
}

static VALUE
writer_jpeg_write(VALUE obj, VALUE path)
{
    GetImg(obj, data, im);

    if (im_vips2jpeg(im, RSTRING_PTR(path)))
        vips_lib_error();

    return obj;
}

static VALUE
writer_tiff_write(VALUE obj, VALUE path)
{
    GetImg(obj, data, im);

    if (im_vips2tiff(im, RSTRING_PTR(path)))
        vips_lib_error();

    return obj;
}

static VALUE
writer_ppm_write(VALUE obj, VALUE path)
{
    GetImg(obj, data, im);

    if (im_vips2ppm(im, RSTRING_PTR(path)))
        vips_lib_error();

    return obj;
}

#if IM_MAJOR_VERSION > 7 || IM_MINOR_VERSION >= 23

static VALUE
writer_png_buf(VALUE obj, VALUE compression, VALUE interlace)
{
    VipsImage *im_out;
    char *buf;
    int length;
    GetImg(obj, data, im);

    if (!(im_out = im_open("writer_png_buf", "p")))
        vips_lib_error();

    if (im_vips2bufpng(im, im_out, NUM2INT(compression), NUM2INT(interlace),
        &buf, &length)) {
		im_close(im_out);
        vips_lib_error();
	}

    im_close(im_out);

    return rb_tainted_str_new(buf, length);
}

#endif

static VALUE
writer_png_write(VALUE obj, VALUE path)
{
    GetImg(obj, data, im);

    if (im_vips2png(im, RSTRING_PTR(path)))
        vips_lib_error();

    return obj;
}

static VALUE
writer_csv_write(VALUE obj, VALUE path)
{
    GetImg(obj, data, im);

    if (im_vips2csv(im, RSTRING_PTR(path)))
        vips_lib_error();

    return obj;
}

void
init_writer()
{
    cVIPSWriter = rb_define_class_under(mVIPS, "Writer", rb_cObject);
    rb_define_alloc_func(cVIPSWriter, img_init_partial_anyclass);
    rb_define_method(cVIPSWriter, "initialize", writer_initialize, 1);
    rb_define_method(cVIPSWriter, "image", writer_image, 0);
    rb_define_method(cVIPSWriter, "exif", writer_exif, 0);
    rb_define_method(cVIPSWriter, "exif?", writer_exif_p, 0);
    rb_define_method(cVIPSWriter, "exif=", writer_exif_set, 1);
    rb_define_method(cVIPSWriter, "remove_exif", writer_remove_exif, 0);
    rb_define_method(cVIPSWriter, "icc", writer_icc, 0);
    rb_define_method(cVIPSWriter, "icc?", writer_icc_p, 0);
    rb_define_method(cVIPSWriter, "icc=", writer_icc_set, 1);
    rb_define_method(cVIPSWriter, "remove_icc", writer_remove_icc, 0);

    rb_define_private_method(cVIPSWriter, "jpeg_buf", writer_jpeg_buf, 1);
#if IM_MAJOR_VERSION > 7 || IM_MINOR_VERSION >= 23
    rb_define_private_method(cVIPSWriter, "png_buf", writer_png_buf, 2);
#endif
    rb_define_private_method(cVIPSWriter, "jpeg_write", writer_jpeg_write, 1);
    rb_define_private_method(cVIPSWriter, "tiff_write", writer_tiff_write, 1);
    rb_define_private_method(cVIPSWriter, "ppm_write", writer_ppm_write, 1);
    rb_define_private_method(cVIPSWriter, "png_write", writer_png_write, 1);
    rb_define_private_method(cVIPSWriter, "csv_write", writer_csv_write, 1);
}
