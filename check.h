#ifndef CHECK_H
#define CHECK_H


//�ýṹ����sqlע����Խ�����������
struct url_query
{
  char * url;    //URL����
  int query_nums; //������Ŀ
  struct url_query_node ** query_node; //�ָ�����Ĳ����ṹָ������
};

//�ýṹ����sqlע����Խ�������������URL�е�ƫ��
struct url_query_node                
{
  int value_star;                    //������string�е���ʼƫ�ƣ���0����
  int value_end;                     //������string�еĽ���ƫ�ƣ���0����
};

//�ýṹ����sqlע����Ա��淵�ؽ��
struct sql_injection_result
{
    int isok; //�����Ƿ�ɹ�
	int len;  //�������ݳ���
	char *url;//���ɵ��µ������ַ��ȫ·��
	char *query;//�������
	char* result; //��������
};
#endif
